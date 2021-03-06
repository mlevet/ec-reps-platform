<?php

/**
 * @file
 * Extension of the Views Plugin Style for Mapeditor Map.
 *
 * Adapted from the Leaflet Views module.
 */

/**
 * Defines views style.
 */
class MapeditorViewsPluginStyle extends views_plugin_style {

  /**
   * Save the entity type and info if this view is displaying an entity.
   */
  public function init(&$view, &$display, $options = NULL) {
    parent::init($view, $display, $options);
    foreach (entity_get_info() as $key => $info) {
      if ($view->base_table == $info['base table']) {
        $this->entity_type = $key;
        $this->entity_info = $info;
        break;
      }
    }
  }

  /**
   * Sets default options.
   */
  public function optionDefinition() {
    $options = parent::optionDefinition();
    $options['data_source'] = array('default' => '');
    $options['view_mode'] = array('default' => 'popup');
    $options['settings'] = array();
    $options['tile_layer'] = array('default' => 'osmec');
    $options['height'] = array('default' => '430');
    $options['center']['contains'] = array(
      'lat' => array('default' => 50.84),
      'lon' => array('default' => 4.36),
    );
    $options['popup']['contains'] = array(
      'show' => array('default' => TRUE),
      'popin' => array('default' => TRUE),
    );
    $options['zoom']['contains'] = array(
      'initialZoom' => array('default' => 2),
      'minZoom' => array('default' => 2),
      'maxZoom' => array('default' => 12),
    );
    return $options;
  }

  /**
   * Constructs options form.
   */
  public function optionsForm(&$form, &$form_state) {
    parent::optionsForm($form, $form_state);

    // Gets list of fields in this view & flag available geodata fields.
    $handlers = $this->display->handler->get_handlers('field');

    $fields = array();
    $fields_data = array();
    foreach ($handlers as $field_id => $handler) {
      $fields[$field_id] = $handler->ui_name();
      if (!empty($handler->field_info['type']) && $handler->field_info['type'] == 'geofield') {
        $fields_data[$field_id] = $handler->ui_name();
      }
    }

    // Checks whether there is a geofield we can work with.
    if (!count($fields_data)) {
      $form['error'] = array(
        '#markup' => t('Please add at least one geofield to the view'),
      );
      return;
    }

    // Defines data source field.
    $form['data_source'] = array(
      '#type' => 'select',
      '#title' => t('Data Source'),
      '#description' => t('Which field contains geodata?'),
      '#options' => $fields_data,
      '#default_value' => $this->options['data_source'],
      '#required' => TRUE,
    );

    // Creates view mode options.
    if ($this->entity_type) {

      // Gets the labels (human readable) of the view modes.
      $view_mode_options = array();
      foreach ($this->entity_info['view modes'] as $key => $view_mode) {
        $view_mode_options[$key] = $view_mode['label'];
      }

      // Outputs the form.
      $form['view_mode'] = array(
        '#type' => 'select',
        '#title' => t('View mode'),
        '#description' => t('View mode to be used in the popup.'),
        '#options' => $view_mode_options,
        '#default_value' => !empty($this->options['view_mode']) ? $this->options['view_mode'] : 'full',
        '#states' => array(
          'visible' => array(
            ':input[name="style_options[description_field]"]' => array('value' => '#rendered_entity'),
          ),
        ),
      );
    }

    $form['hide_empty'] = array(
      '#title' => t('Hide empty'),
      '#type' => 'checkbox',
      '#description' => t('Hide the Mapeditor map if there are no results to display.'),
      '#default_value' => isset($this->options['hide_empty']) ? $this->options['hide_empty'] : TRUE,
    );

    // Fetches more settings form elements.
    $form['tile_layer'] = _mapeditor_form_elements('tile_layer', $this->options);
    $form['height'] = _mapeditor_form_elements('height', $this->options);
    $form['popup'] = _mapeditor_form_elements('popup', $this->options);
    $form['zoom'] = _mapeditor_form_elements('zoom', $this->options);
    $form['center'] = _mapeditor_form_elements('center', $this->options);
    $form['icon'] = _mapeditor_form_elements('icon', $this->options, array(
      'path' => 'style_options',
      'fields' => $fields,
    ));
  }

  /**
   * Renders view.
   */
  public function render() {
    if (!empty($this->view->live_preview)) {
      return t('No preview available');
    }

    // @todo. refactor. rename features to data.
    $features = array();
    $all_coordinates = array();

    // Checks if there is a geofield and add coordinates.
    if ($this->options['data_source']) {
      $this->render_fields($this->view->result);
      foreach ($this->view->result as $id => $result) {
        $geofield = $this->get_field_value($id, $this->options['data_source']);

        if (!empty($geofield)) {
          $entity = FALSE;

          // Renders the entity with the selected view mode.
          if (is_object($result)) {
            $entity = entity_load_single($this->entity_type, $result->{$this->entity_info['entity keys']['id']});
            $build = entity_view($this->entity_type, array($entity), $this->options['view_mode']);
            $description = drupal_render($build);
          }
          $points = _mapeditor_process_geofield($geofield);
          $features[] = array(
            'type' => 'Feature',
            'properties' => array(
              'name' => $entity->title,
              'popupContent' => $description,
            ),
            'geometry' => array(
              'type' => $points[0]['gj_type'],
              'coordinates' => array(
                round($points[0]['lon'], 4),
                round($points[0]['lat'], 4),
              ),
            ),
          );

          // Collects all coordinates (lat, lon) for the fitBounds method.
          $all_coordinates[] = array(
            round($points[0]['lat'], 4),
            round($points[0]['lon'], 4),
          );
        }
      }

      // Fetches default map settings.
      $map_defaults = _mapeditor_map_defaults();
      $map = $map_defaults['map'];
      $settings = $map_defaults['settings'];

      // Applies options from view.
      $map['map'] = $this->options['tile_layer']['tile_layer'];
      $map['minZoom'] = $this->options['zoom']['minZoom'];
      $map['maxZoom'] = $this->options['zoom']['maxZoom'];
      $map['zoom'] = $this->options['zoom']['initialZoom'];
      $map['center'] = array(
        $this->options['center']['lat'],
        $this->options['center']['lon'],
      );
      $settings['popup'] = $this->options['popup']['popin'];
      $settings['show_popup'] = $this->options['popup']['show'];
      $settings['icon'] = $this->options['icon']['icon'];
      $settings['height'] = 430;
      $settings['all_coordinates'] = $all_coordinates;
      if (empty($features) && !empty($this->options['hide_empty'])) {
        return '';
      }

      $mapeditor_map = new MapeditorMapBuild($map, $settings);
      $mapeditor_map->setCustomJs('map.js');
      $mapeditor_map->setFeatures($features);
      return $mapeditor_map->build;
    }
    return '';
  }

}
