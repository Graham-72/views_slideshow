<?php

/**
 * @file
 * The theme system, which controls the output of views slideshow.
 *
 * This just adds a wrapper div to the slideshow.
 */

function _views_slideshow_preprocess_views_slideshow(&$vars) {
  $options = $vars['options'];
  $vars['skin'] = 'default';
  $vars['slideshow'] = '';
  $main_frame_module = $options['slideshow_type'];

  if (empty($main_frame_module)) {
    // Get all slideshow types.
    $slideshows = module_invoke_all('views_slideshow_slideshow_info');

    if ($slideshows) {
      foreach ($slideshows as $slideshow_id => $slideshow_info) {
        $main_frame_module = $slideshow_id;
        break;
      }
    }
  }

  // Make sure the main slideshow settings are defined before building the
  // slideshow.
  if (empty($main_frame_module)) {
    backdrop_set_message(t('No main frame module is enabled for views slideshow. '
      . 'This is often because another module which Views Slideshow needs is not enabled. '
      . 'For example, 3.x needs a module like "Views Slideshow: Cycle" enabled.'), 'error');
  }
  elseif (empty($options[$main_frame_module])) {
    backdrop_set_message(t('The options for !module does not exists.', array('!module' => $main_frame_module)), 'error');
  }
  elseif (!empty($vars['rows'])) {
    $settings = $options[$main_frame_module];
    $view = $vars['view'];
    $rows = $vars['rows'];
    $vss_id = $view->name . '-' . $view->current_display;

    // Give each slideshow a unique id if there are more than one on the page.
    static $instances = array();
    if (isset($instances[$vss_id])) {
      $instances[$vss_id]++;
      $vss_id .= "_" . $instances[$vss_id];
    }
    else {
      $instances[$vss_id] = 1;
    }

    // Building our default methods.
    $methods = array(
      'goToSlide' => array(),
      'nextSlide' => array(),
      'pause' => array(),
      'play' => array(),
      'previousSlide' => array(),
      'transitionBegin' => array(),
      'transitionEnd' => array(),
    );

    // Pull all widget info and slideshow info and merge them together.
    $widgets = module_invoke_all('views_slideshow_widget_info');
    $slideshows = module_invoke_all('views_slideshow_slideshow_info');
    $addons = array_merge($widgets, $slideshows);

    // Loop through all the addons and call their methods if needed.
    foreach ($addons as $addon_id => $addon_info) {
      foreach ($addon_info['accepts'] as $imp_key => $imp_value) {
        if (is_array($imp_value)) {
          $methods[$imp_key][] = views_slideshow_format_addons_name($addon_id);
        }
        else {
          $methods[$imp_value][] = views_slideshow_format_addons_name($addon_id);
        }
      }
    }

    $js_settings = array(
      'viewsSlideshow' => array(
        $vss_id => array(
          'methods' => $methods,
          'paused' => 0,
        )
      )
    );
    backdrop_add_library('views_slideshow', 'views_slideshow');
    backdrop_add_js($js_settings, 'setting');

    /**
     * Process Skins
     */
    $skin_info = array();
    if (isset($options['skin_info'])) {
      $skin_info = $options['skin_info'];
    }

    // Make sure $skin_info has all the values.
    $skin_info += array(
      'class' => 'default',
      'name' => t('Untitled skin'),
      'module' => 'views_slideshow',
      'path' => '',
      'stylesheets' => array(),
    );

    $vars['skin'] = $skin_info['class'];

    // Enqueue any stylesheets set for the skin on this view are added.
    $skin_path = backdrop_get_path('module', $skin_info['module']);
    if ($skin_info['path']) {
      $skin_path .= '/' . $skin_info['path'];
    }

    // Add stylesheet.
    if (!empty($skin_info['stylesheets'])) {
      foreach ($skin_info['stylesheets'] as $stylesheet) {
        backdrop_add_css($skin_path . '/' . $stylesheet);
      }
    }

    /**
     * Process Widgets
     */

    // Build weights
    for ($i = 1; $i <= count($widgets); $i++) {
      $weight['top'][$i] = array();
      $weight['bottom'][$i] = array();
    }

    foreach ($widgets as $widget_id => $widget_name) {
      // Put our widgets in the right location.
      if ($options['widgets']['top'][$widget_id]['enable']) {
        $widget_weight = ($options['widgets']['top'][$widget_id]['weight'] > count($widgets)) ? count($widgets) : $options['widgets']['top'][$widget_id]['weight'];
        $weight['top'][$widget_weight][] = $widget_id;
      }

      if ($options['widgets']['bottom'][$widget_id]['enable']) {
        $widget_weight = ($options['widgets']['bottom'][$widget_id]['weight'] > count($widgets)) ? count($widgets) : $options['widgets']['bottom'][$widget_id]['weight'];
        $weight['bottom'][$widget_weight][] = $widget_id;
      }
    }

    // Build our widgets
    foreach ($weight as $location => $order) {
      $vars[$location . '_widget_rendered'] = '';
      foreach ($order as $order_num => $widgets) {
        if (is_array($widgets)) {
          foreach ($widgets as $widget_id) {
            $vars[$widget_id . '_' . $location] = theme(views_theme_functions($widget_id . '_widget_render', $view, $view->display[$view->current_display]), array('vss_id' => $vss_id, 'view' => $view, 'settings' => $options['widgets'][$location][$widget_id], 'location' => $location, 'rows' => $rows));
            $vars[$location . '_widget_rendered'] .= $vars[$widget_id . '_' . $location];
          }
        }
      }
    }

    /**
     * Process Slideshow
     */
    $slides = theme(views_theme_functions($main_frame_module . '_main_frame', $view, $view->display[$view->current_display]), array('vss_id' => $vss_id, 'view' => $view, 'settings' => $settings, 'rows' => $rows, 'options' => $options,));
    $vars['slideshow'] = theme(views_theme_functions('views_slideshow_main_section', $view, $view->display[$view->current_display]), array('vss_id' => $vss_id, 'slides' => $slides, 'plugin' => $main_frame_module));
  }
}

/**
 * The current element of the slideshow.
 *
 * @ingroup themeable
 */
function theme_views_slideshow_main_section($vars) {
  return '<div id="' . $vars['plugin'] . '_main_' . $vars['vss_id'] . '" class="' . $vars['plugin'] . '_main views_slideshow_main">' . $vars['slides'] . '</div>';
}

/**
 * Views Slideshow: pager.
 *
 * @ingroup themeable
 */
function theme_views_slideshow_pager_widget_render($vars) {
  if (isset($vars['settings']['hide_on_single_slide']) && $vars['settings']['hide_on_single_slide'] === 1 && count($vars['rows']) < 2) {
    return '';
  }

  // Add JavaScript settings for the pager type.
  $js_vars = array(
    'viewsSlideshowPager' => array(
      $vars['vss_id'] => array(
        $vars['location'] => array(
          'type' => views_slideshow_format_addons_name($vars['settings']['type'])
        ),
      ),
    ),
  );

  backdrop_add_library('views_slideshow', 'views_slideshow');
  backdrop_add_js($js_vars, 'setting');

  // Create some attributes
  $vss_attributes['class'] = 'widget_pager widget_pager_' . $vars['location'];
  $vss_attributes['id'] = 'widget_pager_' . $vars['location'] . '_' . $vars['vss_id'];
  return theme(views_theme_functions($vars['settings']['type'], $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings'], 'location' => $vars['location'], 'vss_attributes' => $vss_attributes));
}

/**
 * Theme pager fields
 */
function _views_slideshow_preprocess_views_slideshow_pager_fields(&$vars) {
  // Build our JavaScript settings.
  $js_vars = array(
    'viewsSlideshowPagerFields' => array(
      $vars['vss_id'] => array(
        $vars['location'] => array(
          'activatePauseOnHover' => $vars['settings']['views_slideshow_pager_fields_hover'],
        ),
      ),
    ),
  );

  // Add the settings to the page.
  backdrop_add_library('views_slideshow', 'views_slideshow');
  backdrop_add_js($js_vars, 'setting');

  // Add hover intent library
  if ($vars['settings']['views_slideshow_pager_fields_hover']) {
    if (module_exists('libraries')) {
      // Load jQuery hoverIntent
      $hoverIntent_path = libraries_get_path('jquery.hoverIntent');
      if (!empty($hoverIntent_path) && file_exists($hoverIntent_path . '/jquery.hoverIntent.js')) {
        backdrop_add_js($hoverIntent_path . '/jquery.hoverIntent.js');
      }
    }
  }

  $vars['classes'][] = $vars['vss_attributes']['class'];
  $vars['widget_id'] = $vars['vss_attributes']['id'];
  // Add our class to the wrapper.
  $vars['classes'][] = 'views_slideshow_pager_field';

  // Render all the fields unless there is only 1 slide and the user specified
  // to hide them when there is only one slide.
  $vars['rendered_field_items'] = '';
  if (empty($vars['settings']['hide_on_single_slide']) || count($vars['view']->result) > $vars['view']->style_options['views_slideshow_cycle']['items_per_slide']) {
    foreach ($vars['view']->result as $count => $node) {
      $rendered_fields = '';
      foreach ($vars['settings']['views_slideshow_pager_fields_fields'] as $field => $use) {
        if ($use !== 0 && is_object($vars['view']->field[$field])) {
          $rendered_fields .= theme(views_theme_functions('views_slideshow_pager_field_field', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('view' => $vars['view'], 'field' => $field, 'count' => $count));
        }
      }
      $vars['rendered_field_items'] .= theme(views_theme_functions('views_slideshow_pager_field_item', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'item' => $rendered_fields, 'count' => $count, 'location' => $vars['location'], 'length' => count($vars['view']->result)));
    }
  }
}

/**
 * Views Slideshow: pager item.
 *
 * @ingroup themeable
 */
function _views_slideshow_preprocess_views_slideshow_pager_field_item(&$vars) {
  $vars['classes'][] = 'views_slideshow_pager_field_item';
  $vars['classes'][] = ($vars['count'] % 2) ? 'views-row-even' : 'views-row-odd';
  if ($vars['count'] == 0) {
    $vars['classes'][] = 'views-row-first';
  }
  elseif ($vars['count'] == $vars['length'] - 1) {
    $vars['classes'][] = 'views-row-last';
  }
}

/**
 * Views Slideshow: Controls.
 *
 * @inggroup themeable
 */
function theme_views_slideshow_controls_widget_render($vars) {
  // Add JavaScript settings for the controls type.
  $js_vars = array(
    'viewsSlideshowControls' => array(
      $vars['vss_id'] => array(
        $vars['location'] => array(
          'type' => views_slideshow_format_addons_name($vars['settings']['type'])
        ),
      ),
    ),
  );

  backdrop_add_library('views_slideshow', 'views_slideshow');
  backdrop_add_js($js_vars, 'setting');

  $output = '';
  if (empty($vars['settings']['hide_on_single_slide']) || count($vars['rows']) > $vars['view']->style_options['views_slideshow_cycle']['items_per_slide']) {
    $output = theme(views_theme_functions($vars['settings']['type'], $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings'], 'location' => $vars['location'], 'rows' => $vars['rows']));
  }

  return $output;
}

/**
 * The slideshow controls.
 *
 * @ingroup themeable
 */
function _views_slideshow_preprocess_views_slideshow_controls_text(&$vars) {
  $module_path = backdrop_get_path('module', 'views_slideshow');
  backdrop_add_css($module_path . '/views_slideshow_controls_text.css', array('type' => 'file'));

  $vars['classes'][] = 'views_slideshow_controls_text';

  $vars['rendered_control_previous'] = theme(views_theme_functions('views_slideshow_controls_text_previous', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings']));

  $vars['rendered_control_pause'] = theme(views_theme_functions('views_slideshow_controls_text_pause', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings']));

  $vars['rendered_control_next'] = theme(views_theme_functions('views_slideshow_controls_text_next', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings']));
}

/**
 * Views Slideshow: "previous" control.
 *
 * @ingroup themeable
 */
function _views_slideshow_preprocess_views_slideshow_controls_text_previous(&$vars) {
  $vars['classes'][] = 'views_slideshow_controls_text_previous';
}

/**
 * Views Slideshow: "pause" control.
 *
 * @ingroup themeable
 */
function _views_slideshow_preprocess_views_slideshow_controls_text_pause(&$vars) {
  $vars['classes'][]  = 'views_slideshow_controls_text_pause  views-slideshow-controls-text-status-play';
  $vars['start_text'] = t('Pause');
}

/**
 * Views Slideshow: "next" control.
 *
 * @ingroup themeable
 */
function _views_slideshow_preprocess_views_slideshow_controls_text_next(&$vars) {
  $vars['classes'][] = 'views_slideshow_controls_text_next';
}

/**
 * Views Slideshow: Slide Counter.
 *
 * @inggroup themeable
 */
function theme_views_slideshow_slide_counter_widget_render($vars) {
  return theme(views_theme_functions('views_slideshow_slide_counter', $vars['view'], $vars['view']->display[$vars['view']->current_display]), array('vss_id' => $vars['vss_id'], 'view' => $vars['view'], 'settings' => $vars['settings'], 'location' => $vars['location'], 'rows' => $vars['rows']));
}

/**
 * Views Slideshow: slide counter.
 */
function _views_slideshow_preprocess_views_slideshow_slide_counter(&$vars) {
  $vars['classes'][] = 'views_slideshow_slide_counter';
  $vars['slide_count'] = count($vars['rows']);
}
