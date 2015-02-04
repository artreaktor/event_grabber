<?php

//Collect links to be parsed
function viralagenda_handler($grabber) {
  //dsm('Handler invoked ' . $url);

  $html = event_grabber_get_web_page($grabber->url);
  //dsm($html);
  //$evt_url = 'http://www.viralagenda.com/pt/events/138200/concerto-tributo-a-xutos-pontapes'.'?ajax=1';
  //Load all events
  $porto = 'http://www.viralagenda.com/pt/porto?ajax=1&page=10&perpage=3000000';
  $event_data = json_decode(event_grabber_get_web_page($porto));
  //dsm($event_data);

  //Fetch all event ids
  preg_match_all('#<li id="c(.*)"#U', $event_data->html, $id_matches);
  $ids = $id_matches[1];

  //dsm($ids);

  for ($i=1; $i<=10; $i++) {
    viralagenda_processor($ids[$i], $grabber);
  }

  //viralagenda_processor($ids[1]);
  //viralagenda_processor(139877, $grabber);

}


//Saves each single event
function viralagenda_processor($evt_id, $grabber) {
  //EVENT
  $evt_url = 'http://www.viralagenda.com/pt/events/' . $evt_id . '?ajax=1';
  $evt_data = json_decode(event_grabber_get_web_page($evt_url));
  event_grabber_init();
  $evt_html = str_get_html('<html><body>' . $evt_data->html . '</html></body>');
  //IMAGE
  $img = $evt_html->find('img.cover-img', 0)->src;
  // Avoid saiving default 'no-picture' image
  if (strpos($img, 'event-default.png')) {
    $img = '';
  }
  //MAP
  $map_url = 'http://www.viralagenda.com/maplist';
  $post = 'evt=' . $evt_id;
  $map_data = json_decode(event_grabber_get_web_page($map_url, $post));
  $map = array_shift($map_data);

  //START DATE
  $date_start = $evt_html->find('time[itemprop="startDate"]', 0)->datetime;
  $date = date_create_from_format('Y-m-d\TH:i:s', str_replace('+00:00', '', $date_start));
  $start = $date->getTimestamp();
  $conv_timezone = event_grabber_convert_timezone($map->latitude, $map->longitude, $start);
  $conv_start_date = $conv_timezone['dest_time'];
  //END DATE
  $date_end = $evt_html->find('time[itemprop="endDate"]', 0)->datetime;
  if ($date_end != '') {
    $date = date_create_from_format('Y-m-d\TH:i:s', str_replace('+00:00', '', $date_end));
    $end = $date->getTimestamp();
  }
  //If end time is not defined calculate it as $start + 2 hours
  else {
    $end = $start+7200;
  }
  $conv_end_date = $end + $conv_timezone['offset'];

  $event = array(
    'title' => str_replace(' - Viral Agenda', '', $evt_data->title),
    'description' => $evt_data->description,
    'location' => $map->page_name . ', ' . $map->street . ', ' . $map->city,
    'date_start_ts' => $conv_start_date,
    'date_start' => date('d-m-Y H:i', $conv_start_date),
    'date_end_ts' => $conv_end_date,
    'date_end' => date('d-m-Y H:i', $conv_end_date),
    'images' => array($img),
    'latitude' => $map->latitude,
    'longitude' => $map->longitude,
    'source_id' => $evt_id,
    'grabber' => $grabber,
  );

  //event_grabber_save_event($event);
  dsm($event['title'] . ' ' . $event['date_start']);

}















