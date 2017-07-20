<?php

/**
 * @file
 * Memberform definition and validation and submit functions.
 */

/**
 * Form constructor for the member form.
 */
function memberform_form($form, &$form_state) {
  $values = isset($form_state['values']) ? $form_state['values'] : array();
  $storage = memberform_get_storage();

  // Set default page.
  if (!isset($storage['current_page'])) {
    $storage['current_page'] = 'start';
    $storage['step'] = 1;
  }

  // Store storage data in form.
  $form_state['storage'] = $storage;
  $storage = &$form_state['storage'];
  dpm($storage);

  // Create form based on form page.
  $form = array(
    '#validate' => array(
      'memberform_form_validate',
      'memberform_page_form_validate',
    ),
    '#submit' => array(
      'memberform_page_form_submit',
      'memberform_form_submit',
    ),
    '#prefix' => '<div id="memberform_form_wrapper">',
    '#suffix' => '</div>',
  );

  $form['progress'] = array(
    '#markup' => '<h3>Stap ' . $storage['step'] . '</h3>',
  );

  // Get pages forms.
  $form_callback = 'memberform_page_' . $storage['current_page'] . '_form';
  if (function_exists($form_callback)) {

    // Add page specific form elements.
    $form_callback($form, $storage, $values);

    // Modify form elements.
    foreach ($form as $key => $item) {
      if (preg_match('/^page_' . $storage['current_page'] . '_[a-z0-9_]+$/', $key, $matches)) {
        // Add default values.
        if (isset($storage['values'][$storage['current_page']][$key])) {
          $form[$key]['#default_value'] = $storage['values'][$storage['current_page']][$key];
        }
        // Add error messages.
        if (isset($form_state['form_errors'][$key])) {
          $form[$key]['#prefix'] = '<div class="memberform validation-error"><div class="messages error">' . $form_state['form_errors'][$key] . '</div>';
        }
        else {
          $form[$key]['#prefix'] = '<div class="memberform">';
        }
        $form[$key]['#suffix'] = '</div>';
        // Rebuild required functionality.
        if (!empty($form[$key]['#required'])) {
          unset($form[$key]['#required']);
          $form[$key]['#title'] .= ' <span class="form-required" title="This field is required.">*</span>';
          $storage['required_fields'][$key] = $key;
        }
      }
    }
  }

  // Add action buttons.
  if ($storage['step'] > 1) {
    $no_back_pages = memberform_settings('no_back_pages');
    if (!in_array($storage['current_page'], $no_back_pages)) {
      $form['previous_page'] = array(
        '#type' => 'submit',
        '#value' => 'Vorige',
        '#name' => 'previous_page',
        '#ajax' => array(
          'wrapper' => 'memberform_form_wrapper',
          'callback' => 'memberform_ajax_callback',
        ),
      );
    }
  }

  if (!empty($storage['last_page'])) {
    $form['finish'] = array(
      '#type' => 'submit',
      '#value' => 'Insturen',
      '#name' => 'finish',
    );
  }
  else {
    $form['next_page'] = array(
      '#type' => 'submit',
      '#value' => 'Volgende',
      '#name' => 'next_page',
      '#ajax' => array(
        'wrapper' => 'memberform_form_wrapper',
        'callback' => 'memberform_ajax_callback',
      ),
    );
  }

  return $form;
}

/**
 * Validate function for memberform.
 */
function memberform_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $storage = &$form_state['storage'];

  // Store values.
  $storage['values'][$storage['current_page']] = $values;
  memberform_store_form_data($storage);

  // Implement required fields validation.
  $trigger = $form_state['triggering_element']['#name'];
  if ($trigger !== 'previous_page') {
    foreach ($values as $key => $value) {
      if (isset($storage['required_fields'][$key])) {
        if (empty($value)) {
          form_set_error($key, $form[$key]['#title'] . ' is verplicht!');
        }
      }
    }
  }
}

/**
 * Submit function for memberform.
 */
function memberform_form_submit($form, &$form_state) {

  // Rebuild form when errors occured.
  if (isset($form_state['form_errors'])) {
    $form_state['rebuild'] = TRUE;
    return;
  }

  $values = $form_state['values'];
  $storage = &$form_state['storage'];

  // Store values.
  $storage['values'][$storage['current_page']] = $values;

  // Process previous, next page.
  $trigger = $form_state['triggering_element']['#name'];

  switch ($trigger) {
    case 'previous_page':
      $storage['step']--;
      $storage['current_page'] = array_pop($storage['page_history']);
      unset($storage['last_page']);
      unset($storage['next_page']);
      break;

    case 'next_page':
      $storage['step']++;
      $storage['page_history'][] = $storage['current_page'];
      $storage['current_page'] = $storage['next_page'];
      break;
  }

  // Store form data in database.
  memberform_store_form_data($storage);

  // Store submission id in session.
  $_SESSION['memberform']['sid'] = $storage['sid'];

  // Rebuild form when not finished.
  if ($trigger !== 'finish') {
    $form_state['rebuild'] = TRUE;
  }
  else {
    unset($_SESSION['memberform']['sid']);
  }
}

/**
 * Ajax callback function for memberform.
 */
function memberform_ajax_callback($form, &$form_state) {
  return $form;
}