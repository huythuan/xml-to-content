<?php

/**
 * Admin Settings form
 */
 
function xml_to_content_settings() {
  $output = l('Create New XML Feed', 'node/add/xml-feed', array('query' => drupal_get_destination()));
  
  // create a table of the available feeds and related info
  
  $data = array();
  
  //set the table row headers
  $headers = array(array('data' => 'Title', 'field' => 'title', 'sort' => 'asc'), array('data' => 'File', 'field' => 'filename'), array('data' => 'Maps (Node Type)', 'field' => 'node_type'), array('data' => 'Active', 'field' => 'node_type'), array('data' => 'Actions'));
  
  // Show a table of all of the feeds that have been created
  $output .= "<p><strong>Available Feeds:</strong></p>";
  
  // get all the xml_feed nodes
  $node_type = "xml_feed";
  $result = db_query("SELECT nid FROM node WHERE type = '%s' ", $node_type);
  while ($obj = db_fetch_object ($result)) {
    $node = node_load($obj->nid);
    $title_link = l($node->title, 'node/' . $node->nid, array('attributes' => array('class' => 'node-link')));
    $edit_link = l(t('Edit'), 'node/' . $node->nid . '/edit', array('attributes' => array('class' => 'node-link'), 'query' => drupal_get_destination()));
    $map_link = l(t('Mapping'), 'admin/settings/xml_to_content/mapping/' . $node->nid, array('attributes' => array('class' => 'node-link')));
    $parse_link = l(t('Parse'), 'xml2content/' . $node->nid, array('attributes' => array('class' => 'node-link', 'target' => '_blank')));
    $text_status = ($node->status)? 'Yes' : 'No';
    $fname = l(t('View File'), $node->filename, array('attributes' => array('class' => 'link-class', 'target' => '_blank')));
    $data[] = array($title_link, $fname, $node->node_type, $text_status, $edit_link . ' | '. $map_link . ' | '. $parse_link);
  }
  
  //theme the data and headers as a table
  $output .= theme_table($headers,$data);
  
  return $output;
}

function xml_to_content_mapping_form($form_state,$node) {
  $form = array();

  $field_options = array();
  $vocab_options = array();
  
  //get all of the fields of the content type selected
  $node_type = node_get_types('type', $node->type);

  $vocab_options[''] = 'NONE';
  $vocabs = taxonomy_get_vocabularies();
  foreach($vocabs as $vocab) {
    $vocab_options[$vocab->vid] = $vocab->name;
  }
  
  $form['mapping-for-'] = array(
    '#value' => 'Mapping for '.$node->title,
    '#prefix' => '<div class="mapping-header"><h2>',
    '#suffix' => '</h2></div>',
  );
  
  // Get the previous mapping values if they exist
  $update = 0;
  $mapping_sql = "SELECT mappings FROM {xml_to_content_map} WHERE nid = %d";
  $mapping_result = db_query($mapping_sql, $node->nid);
  $mappings = db_fetch_object($mapping_result);
  // If there are mapping values unserialize them so they can be used to repopulate the fields.
  // Also, flag this as an update so new data is not inserted.
  if(!empty($mappings->mappings)) {
    $update = 1;
    $map_form_values = unserialize($mappings->mappings);
  }
  
  // No selection option
  $field_options[''] = 'NONE';

  $field_options['title'] = $node_type->title_label;
  
  if($node_type->has_body) {
    $field_options['body'] = $node_type->body_label;
  }
  
  $fields = content_fields(NULL,$node->type);
  foreach($fields as $field) {
    $field_options[$field['field_name']] = $field['field_name'];
  }

  // For each label in the xml file provide a select element to choose a content type field
  $xml_labels = unserialize($node->fields);

  foreach($xml_labels as $label) {
    $form[$label.'_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t($label),
      '#description' => t('Set the mapping for '.$label),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#weight' => 1,
    );

    $form[$label.'_fields'][$label.'_field'] = array(
      '#type' => 'select',
      '#title' => t($label.' Maps to'),
      '#multiple' => FALSE,
      '#description' => t('Select the ' . $node_type->name . ' field to map.'),
      '#options' => $field_options,
      '#default_value' => $map_form_values[$label.'_field'],
      '#weight' => 2,
    );

    $form[$label.'_fields'][$label.'_maps_taxonomy'] = array(
      '#type' => 'checkbox',
      '#title' => t('Maps to Taxonomy'),
      '#default_value' => $map_form_values[$label.'_maps_taxonomy'],
      '#weight' => 3,
    );

    $form[$label.'_fields'][$label.'_taxonomy'] = array(
      '#type' => 'select',
      '#title' => t($label.' Maps to'),
      '#multiple' => FALSE,
      '#description' => t('Select the Taxonomy to map.'),
      '#options' => $vocab_options,
      '#default_value' => $map_form_values[$label.'_taxonomy'],
      '#weight' => 4,
    );

    $form[$label.'_fields'][$label.'_process_file'] = array(
      '#type' => 'checkbox',
      '#title' => t('Process as a File'),
      '#default_value' => $map_form_values[$label.'_process_file'],
      '#weight' => 5,
    );
    
    $form[$label.'_fields'][$label.'_multiple_image_container_field'] = array(
      '#type' => 'textfield',
      '#title' => t('Multiple Image Container'),
      '#description' => t('If there are multiple images enter the name of the element that contains them here (e.g: images)'),
      '#default_value' => $map_form_values[$label.'_multiple_image_container_field'],
      '#weight' => 6,
    );
    
    $form[$label.'_fields'][$label.'_unique_id'] = array(
      '#type' => 'checkbox',
      '#title' => t('This is the unique identifier'),
      '#default_value' => $map_form_values[$label.'_unique_id'],
      '#weight' => 7,
    );
    
    $form[$label.'_fields'][$label.'_date_field'] = array(
      '#type' => 'checkbox',
      '#title' => t('Date Field'),
      '#default_value' => $map_form_values[$label.'_date_field'],
      '#weight' => 8,
    );
    
    $form[$label.'_fields'][$label.'_date_mapping'] = array(
      '#type' => 'select',
      '#title' => t('Start and End Date Mapping'),
      '#multiple' => FALSE,
      '#description' => t('Select if this should map to the Start or End date'),
      '#options' => array('value' => 'Start Date', 'value2' => 'End Date'),
      '#default_value' => $map_form_values[$label.'_date_mapping'],
      '#weight' => 9,
    );
    
  }
  
  $form['nid'] = array(
    '#type' => 'value',
    '#value' => $node->nid
  );
  
  $form['update'] = array(
    '#type' => 'value',
    '#value' => $update
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 100,
  );
    
  return $form;
  
}

function xml_to_content_mapping($node) {
  if($node->type == 'xml_feed') {
    return drupal_get_form('xml_to_content_mapping_form',$node);
  }
  else {
    $output = "Click the XML to Content tab and select a feed.";
    return $output;
  }
}

function xml_to_content_mapping_form_submit($form, &$form_state) {
  $nid = (int)$form_state['values']['nid'];
  $mappings = serialize($form_state['values']);
  
  if(!empty($form_state['values']['update'])) {
    $sql = "UPDATE {xml_to_content_map} SET mappings = '%s' WHERE nid = %d";
    $result = db_query($sql, array($mappings, $nid));
  }
  else {
    $sql = "INSERT INTO {xml_to_content_map} (nid, mappings) VALUES (%d, '%s')";
    $result = db_query($sql, array($nid, $mappings));
  }
  
  if($result) {
    drupal_set_message(t('Mappings have been saved'));
  }
  else {
    drupal_set_message(t('Problem saving mappings'), 'error');
  }
}



