<?php

/**
 * @file
 * Contains forms for the Single profile tools module.
 */

/**
 * Constructs Mapeditor settings form.
 */
function mapeditor_settings() {
  $form = array();

  // Defines input field to change the location of the JavaScript library.
  $form['webtools_environment'] = array(
    '#type' => 'textfield',
    '#title' => t('Webtools environment'),
    '#default_value' => variable_get('mapeditor_webtools_environment', MAPEDITOR_WEBTOOLS_ENVIRONMENT),
    '#description' => t('Webtools environment from where to use the load.js library'),
    '#size' => '60',
  );

  $form['mapeditor_data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Mapeditor data'),
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
  );

  // Populates dropdown with system's content types. But excludes the Map
  // content type to prevent confusion.
  $node_types = node_type_get_names();
  if (array_key_exists('mapeditor_map', $node_types)) {
    unset($node_types['mapeditor_map']);
  }
  $form['mapeditor_data']['mapeditor_node_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content types'),
    '#options' => $node_types,
    '#default_value' => variable_get('mapeditor_node_types', array()),
    '#description' => t('Content types that are available as data sources in the Mapeditor map node.'),
  );

  // Allows including unpublished content in the query.
  // @todo move to mapeditor node level, because it is stupid here.
  $form['mapeditor_show_unpublished'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show unpublished'),
    '#default_value' => variable_get('mapeditor_show_unpublished', FALSE),
    '#description' => t('Show unpublished nodes in a map'),
  );

  // Populates dropdown with system's vocabularies. But excludes the NUTS
  // regions to prevent confusion.
  $taxonomy_vocabularies = taxonomy_get_vocabularies();
  $options = array();
  foreach ($taxonomy_vocabularies as $vocabulary) {
    $options[$vocabulary->machine_name] = $vocabulary->name;
  }
  if (array_key_exists('nuts_regions', $options)) {
    unset($options['nuts_regions']);
  }
  $form['mapeditor_data']['mapeditor_vocabularies'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Vocabularies'),
    '#options' => $options,
    '#default_value' => variable_get('mapeditor_vocabularies', array()),
    '#description' => t('Vocabularies that are available as filters in the Mapeditor map node.'),
  );
  // @todo add default settings as form elements.
  return system_settings_form($form);
}

/**
 * Helper function to standardize forms between field formatters and others.
 *
 * Returns a fully loaded form element.
 */
function _mapeditor_form_elements($group, $settings, $options = NULL) {
  $form_element = NULL;
  switch ($group) {
    case 'popup':
      $form_element = array(
        '#type' => 'container',
        '#weight' => 10,
      );
      $form_element['show'] = array(
        '#title' => t('Show a popup when markers are clicked.'),
        '#type' => 'checkbox',
        '#default_value' => $settings[$group]['show'],
      );
      $form_element['popin'] = array(
        '#title' => t('Use pop-in (show details in right side bar).'),
        '#type' => 'checkbox',
        '#default_value' => $settings[$group]['popin'],
      );
      break;

    case 'zoom':
      // Define zoom options.
      $zoom_options = array(
        0 => t('0 - Low/Far'),
        18 => t('18 - High/Close'),
      );
      for ($i = 1; $i < 18; $i++) {
        $zoom_options[$i] = $i;
      }
      ksort($zoom_options);

      $form_element = array(
        '#type' => 'container',
        '#weight' => 12,
      );
      $form_element['initialZoom'] = array(
        '#title' => t('Initial zoom level'),
        '#description' => t('The starting zoom level when this map is rendered.  Restricted by min and max zoom settings.'),
        '#type' => 'select',
        '#options' => $zoom_options,
        '#default_value' => isset($settings[$group]['initialZoom']) ? $settings[$group]['initialZoom'] : 2,
        '#element_validate' => array('_mapeditor_validate_zoom'),
      );
      $form_element['minZoom'] = array(
        '#title' => t('Minimum zoom level'),
        '#description' => t('The minimum zoom level allowed. (How far away can you view from?)'),
        '#type' => 'select',
        '#options' => $zoom_options,
        '#default_value' => isset($settings[$group]['minZoom']) ? $settings[$group]['minZoom'] : 1,
        '#element_validate' => array('_mapeditor_validate_zoom'),
      );
      $form_element['maxZoom'] = array(
        '#title' => t('Maximum zoom level'),
        '#description' => t('The maximum zoom level allowed. (How close in can you get?).'),
        '#type' => 'select',
        '#options' => $zoom_options,
        '#default_value' => isset($settings[$group]['maxZoom']) ? $settings[$group]['maxZoom'] : 14,
        '#element_validate' => array('_mapeditor_validate_zoom'),
      );
      break;

    case 'icon':
      $form_element = array(
        '#type' => 'container',
        '#weight' => 14,
      );
      $form_element['icon'] = array(
        '#type' => 'select',
        '#title' => t('Icon'),
        '#default_value' => isset($settings[$group]['icon']) ? $settings[$group]['icon'] : 'blue',
        '#options' => _mapeditor_icons(),
      );
      break;

    case 'tile_layer':
      $form_element = array(
        '#type' => 'container',
        '#weight' => 16,
      );
      $form_element['tile_layer'] = array(
        '#type' => 'select',
        '#title' => t('Tile layer'),
        '#default_value' => isset($settings[$group]['tile_layer']) ? $settings[$group]['tile_layer'] : 'osmec',
        '#options' => _mapeditor_tiles(),
      );
      break;

    case 'height':
      $form_element = array(
        '#type' => 'container',
        '#weight' => 6,
      );
      $form_element['height'] = array(
        '#title' => t('Map height'),
        '#type' => 'textfield',
        '#field_suffix' => t('px'),
        '#size' => 4,
        '#default_value' => isset($settings[$group]['height']) ? $settings[$group]['height'] : '430',
        '#required' => FALSE,
        '#element_validate' => array('_mapeditor_validate_height'),
      );
      break;

    case 'center':

      $form_element = array(
        '#type' => 'fieldset',
        '#title' => t('Map center'),
        '#description' => t('Center of the map. E.g. latitude 50.84 and 4.36 longitude for Brussels'),
        '#weight' => 8,
      );
      $form_element['lat'] = array(
        '#title' => t('Latitude'),
        '#type' => 'textfield',
        '#size' => 10,
        '#default_value' => isset($settings[$group]['lat']) ? $settings[$group]['lat'] : 50.84,
        '#required' => FALSE,
        // @todo do
        // '#element_validate' => array('mapeditor_validate_center'),
      );
      $form_element['lon'] = array(
        '#title' => t('Longitude'),
        '#type' => 'textfield',
        '#size' => 10,
        '#default_value' => isset($settings[$group]['lon']) ? $settings[$group]['lon'] : 4.36,
        '#required' => FALSE,
        // @todo do
        // '#element_validate' => array('mapeditor_validate_center'),
      );
      break;

    case 'attribution':
      $form_element = array(
        '#type' => 'container',
        '#weight' => 16,
      );
      $form_element['attributionControl'] = array(
        '#type' => 'checkbox',
        '#title' => t('Show attribution'),
        '#default_value' => isset($settings[$group]['attributionControl']) ? $settings[$group]['attributionControl'] : TRUE,
      );
      $form_element['attribution'] = array(
        '#type' => 'textfield',
        '#title' => t('Attribution text'),
        '#default_value' => isset($settings[$group]['attribution']) ? $settings[$group]['attribution'] : t('Map data by Eurostat and the European Commission'),
        '#size' => 72,
      );
      break;

  }
  return $form_element;
}
