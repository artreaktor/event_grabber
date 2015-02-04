<?php

/**
 * @file
 * Contains administrative pages for creating, editing, and deleting grabber instances.
 */

/**
 * Event grabber administration page.
 */
function event_grabber_admin_page() {

  $grabbers = event_grabber_get_grabbers();
  return theme('event_grabber_admin_page', array(
    'grabbers' => $grabbers,
  ));

}


/**
 *  Admin page theme
 */
function theme_event_grabber_admin_page($variables) {
  $out = '';

  $grabbers = $variables['grabbers'];

  foreach ($grabbers as $grabber) {
    $ops = array(
      'grabber_edit' => array('title' => t('edit'), 'href' => $grabber->admin_path['edit']),
      'grabber_delete' => array('title' => t('delete'), 'href' => $grabber->admin_path['delete']),
    );

    $last_run = ($grabber->last_run > 0) ? date('j M Y H:i:s', $grabber->last_run) : t('Never');
    $manual_form = drupal_get_form('event_grabber_manual_run_form', $grabber);

    $row = array();
    $row[] = check_plain($grabber->name);
    $row[] = $grabber->url;
    $row[] = $last_run;
    $row[] = drupal_render($manual_form);
    $row[] = theme('links', array('links' => $ops));

    $rows[] = array(
      'data' => $row,
    );
  }
  if (!$grabbers) {
    $rows[] = array(
      array('data' => t('No grabbers are currently defined.'), 'colspan' => 5),
    );
  }

  $header = array(
    t('Grabber name'),
    t('Target URL'),
    t('Last run'),
    '',
    t('Operations'),
  );

  $out .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'event_grabber-admin-listing-table'),
  ));

  return $out;
}

function event_grabber_manual_run_form($form, &$form_state, $grabber) {
  $form_state['storage']['grabber'] = $grabber;
  $form = array();

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Run now'),
  );

  return $form;
}

function event_grabber_manual_run_form_submit($form, &$form_state) {
  $grabber = $form_state['storage']['grabber'];
  module_load_include('inc', 'event_grabber', "handlers/$grabber->handler");
  call_user_func_array($grabber->handler, array($grabber));

  //upd last_run field
  db_update('event_grabber')
  ->fields(array('last_run' => time()))
  ->condition('gid', $grabber->gid)
  ->execute();

}

/**
 * Form definition for adding / editing grabber instance.
 */
function event_grabber_entity_form($form, &$form_state, $gid = NULL) {

  $form = array();

  $defaults = array(
    'name' => '',
    'mashine_name' => '',
    'url' => '',
    'cron_job' => '',
    'handler' => '',
  );

  //grabber entity already exists
  if (is_numeric($gid)) {
    $entity = entity_load('event_grabber', array($gid));
    $entity = $entity[$gid];
    $defaults = array(
      'name' => $entity->name,
      'mashine_name' => $entity->mashine_name,
      'url' => $entity->url,
      'cron_job' => $entity->cron_job,
      'handler' => $entity->handler,
    );
    $form_state['storage']['is_new'] = FALSE;
    $form_state['storage']['entity'] = $entity;
  }
  //new entity
  else {
    $form_state['storage']['is_new'] = TRUE;
    $defaults = array(
      'name' => '',
      'mashine_name' => '',
      'url' => '',
      'cron_job' => 0,
      'handler' => '',
    );
  }

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Grabber name'),
    '#descrition' => t('The human-readable name.'),
    '#default_value' => $defaults['name'],
  );
  $form['mashine_name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Mashine name'),
    '#default_value' => $defaults['mashine_name'],
    '#maxlength' => 20,
    '#description' => t('It must only contain lowercase letters, numbers and underscores.'),
    '#machine_name' => array(
      'exists' => 'event_grabber_mashine_name_exists',
      'source' => array('name'),
      'label' => t('Mashine name'),
      'replace_pattern' => '[^a-z0-9_]+',
      'replace' => '_',
    ),
    // Machine name cannot be changed.
    '#disabled' => (!empty($defaults['mashine_name'])),
  );
  $form['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Target URL'),
    '#default_value' => $defaults['url'],
  );
  $form['handler'] = array(
    '#type' => 'select',
    '#title' => t('Select grabber handler'),
    '#description' => t('
      In order to grab new sites you should implement custom
      parser handler and put it into sites/all/modules/event-grabber/handlers.
      Detailed information provided in README.txt.'),
    '#default_value' => $defaults['handler'],
    '#options' => event_grabber_handlers_info(),
  );
  $form['cron_job'] = array(
    '#type' => 'checkbox',
    '#title' => t('Invoke by cron'),
    '#default_value' => $defaults['cron_job'],
    '#description' => 'You can set cron interval in admin/config/system/cron.',
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/structure/event-grabber'),
  );

  return $form;
}

function event_grabber_entity_form_validate($form, &$form_state) {
  if (empty($form_state['values']['name'])) {
    form_set_error('name', t("Status name can't be empty!"));
  }
  if (empty($form_state['values']['url'])) {
    form_set_error('url', t("URL can't be empty!"));
  }
}

function event_grabber_entity_form_submit($form, &$form_state) {

  if ($form_state['storage']['is_new'] == TRUE) {
    $ent = array();
    $ent['name'] = $form_state['values']['name'];
    $ent['mashine_name'] = $form_state['values']['mashine_name'];
    $ent['url'] = $form_state['values']['url'];
    $ent['handler'] = $form_state['values']['handler'];
    $ent['cron_job'] = $form_state['values']['cron_job'];
    $entity = entity_create('event_grabber', $ent);
  }
  else {
    $entity = $form_state['storage']['entity'];
    $entity->name = $form_state['values']['name'];
    $entity->mashine_name = $form_state['values']['mashine_name'];
    $entity->url = $form_state['values']['url'];
    $entity->handler = $form_state['values']['handler'];
    $entity->cron_job = $form_state['values']['cron_job'];
  }
  $entity->save();
  $gid = $entity->gid;

  $form_state['redirect'] = 'admin/structure/event-grabber';
}

function event_grabber_mashine_name_exists($value) {
  return db_query_range('SELECT 1 FROM {event_grabber} WHERE mashine_name = :name', 0, 1, array(':name' => $value))->fetchField();
}

/**
 * Menu callback for adding a new entity status.
 */
function event_grabber_add_page() {
  drupal_set_title(t('Add new target site'));
  return drupal_get_form('event_grabber_entity_form');
}

/*
 * Returns handlers info array
 */
function event_grabber_handlers_info() {
  $files = file_scan_directory(drupal_get_path('module', 'event_grabber'). '/handlers', '/\.inc/');
  $handlers = array();
  foreach ($files as $handler) {
    $handlers[$handler->name] = $handler->name;
  }
  return $handlers;
}


//module_load_include('inc', 'aparser_fieldmap', "handlers/$fieldmap->handler");


/**
 * Event grabber delete form.
 */
function event_grabber_delete_confirm($form, &$form_state, $gid) {
  $ent = entity_load('event_grabber', array($gid));
  $grabber = $ent[$gid];
  $form['#grabber'] = $grabber;
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $grabber->name)),
    !empty($_GET['destination']) ? $_GET['destination'] : 'admin/structure/event-grabber',
    t('Event grabber will be deleted. This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

function event_grabber_delete_confirm_submit($form, &$form_state) {
  $grabber = $form['#grabber'];
  if ($form_state['values']['confirm']) {
    entity_delete('event_grabber', $grabber->gid);
  }
  drupal_set_message(t('Event grabber @name has been deleted.', array('@name' => $grabber->name)));
  $form_state['redirect'] = 'admin/structure/event-grabber';
}

















