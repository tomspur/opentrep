from django.shortcuts import render
from django.http import Http404
from django.http import HttpResponseRedirect, HttpResponse
from django.core.urlresolvers import reverse
from django.utils.encoding import smart_text, smart_bytes
from google.protobuf import text_format
import sys, getopt, os, math
import simplejson as json
import logging
# Potentially add the following two pathes to the PYTHONPATH environment variable
# 1. Path to the OpenTrep C++ library of the Python wrapper (libpyopentrep.so)
# 2. Path to the OpenTrep ProtoBuf stubs (Travel_pb2.py)
import libpyopentrep, Travel_pb2

# 404 - Page not found
def custom_404_view (request):
  #errorMsg = 'Error: page not found'
  return render (request, 'search/404.html', {'error_msg': '',
                                              'request_path': request.path})

# 500 - Server error
def custom_500_view (request):
  #errorMsg = 'Error: Standard Django error'
  return render (request, 'search/500.html', {'error_msg': '',
                                              'request_path': request.path})


# Calculate the great circle distance, given the geographical coordinates
def great_circle_distance (lat1, lon1, lat2, lon2, degrees=True):
  if degrees:
    lat1 = lat1 / 180.0 * math.pi
    lon1 = lon1 / 180.0 * math.pi
    lat2 = lat2 / 180.0 * math.pi
    lon2 = lon2 / 180.0 * math.pi

    diameter = 12756.0
    lat_diff = (lat2-lat1) / 2.0
    lat_diff_sin = math.sin (lat_diff)
    lon_diff = (lon2-lon1) / 2.0
    lon_diff_sin = math.sin (lon_diff)
    lat_cos = math.cos (lat1) * math.cos (lat2)
    proj_dist = lat_diff_sin**2.0 + lat_cos * lon_diff_sin**2.0
    gcd = diameter * math.asin (math.sqrt (proj_dist))
  return gcd

# Calculate the great circle distance of two POR (points of reference) entries
def get_distance_km (place1, place2):
  lat1, lon1 = place1.coord.latitude, place1.coord.longitude
  lat2, lon2 = place2.coord.latitude, place2.coord.longitude
  dist = great_circle_distance (lat1, lon1, lat2, lon2)
  return dist

# Calculate the flight elapsed time
def get_local_local_flight_duration_hr (place1, place2):
  lat1, lon1 = place1.coord.latitude, place1.coord.longitude
  lat2, lon2 = place2.coord.latitude, place2.coord.longitude
  dist = great_circle_distance (lat1, lon1, lat2, lon2)
  travel_hr = 0.5 + dist/800.0
  time_diff_hr = (cit2['lon'] - cit1['lon']) / 15.0
  return travel_hr + time_diff_hr

# Initialise the OpenTrep library
def initOpenTrep():
  # Initialise the OpenTrep C++ library
  xapianDBPath = "/tmp/opentrep/traveldb"
  sqlDBType = "nodb"
  sqlDBConnStr = "/tmp/opentrep/sqlite/ori_por_public.db"
  openTrepLibrary = libpyopentrep.OpenTrepSearcher()
  initOK = openTrepLibrary.init (xapianDBPath, sqlDBType, sqlDBConnStr,
                                 '/tmp/opentrep/pyopentrep.log')
  return initOK, openTrepLibrary

# Extract the answer from the resulting ProtoBuf
def extractAnswer (result):
  # Unserialise the Protobuf
  queryAnswer = Travel_pb2.QueryAnswer()
  queryAnswer.ParseFromString (result)

  # Status
  okStatus = queryAnswer.ok_status

  return okStatus, queryAnswer

# Generate a query string with random place codes
def generateRandomString (nbOfDraws):
  # Initialise the OpenTrep C++ library
  initOK, openTrepLibrary = initOpenTrep()
  if initOK == False:
    return initOK, False, ''

  # Randomly generate one place
  result = openTrepLibrary.generate ('P', nbOfDraws)

  # Extract the answer
  okStatus, queryAnswer = extractAnswer (result)

  # List of places
  placeList = queryAnswer.place_list
  place_list = [place.tvl_code.code for place in placeList.place]

  # Build the query string
  query_string = ' '.join(place_list)

  return initOK, okStatus, query_string


# Extract the parameters from the incoming request
def extract_params (request, query_string = ''):
  #
  result = False
  place_list = None
  zoom_level = 5
  map_type_value = 'HYBRID'

  # Try with GET
  search_form = request.GET

  # Detect the required action
  if search_form.has_key('show_airport'):
    # Randomly generate one place
    initOK, okStatus, query_string = generateRandomString (1)
    if initOK == False:
      errorMsg = 'Error: The OpenTrepLibrary cannot be initialised'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    elif okStatus == False or query_string == '':
      errorMsg = 'Error: The OpenTrepLibrary cannot generate ' + str(nbOfDraws) + ' places'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    result = True

  elif search_form.has_key('show_itinerary'):
    # Randomly generate three places
    initOK, okStatus, query_string = generateRandomString (3)
    if initOK == False:
      errorMsg = 'Error: The OpenTrepLibrary cannot be initialised'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    elif okStatus == False or query_string == '':
      errorMsg = 'Error: The OpenTrepLibrary cannot generate ' + str(nbOfDraws) + ' places'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    result = True

  elif search_form.has_key('q'):
    query_string = search_form['q']
    result = True

  # Detect the Google Map parameters (if any)
  if search_form.has_key('z'):
    zoom_level = search_form['z']
  if search_form.has_key('mt'):
    map_type_value = search_form['mt']

  # Try with POST
  search_form = request.POST

  # Detect the required action
  if search_form.has_key('q'):
    query_string = search_form['q']
    result = True

  elif search_form.has_key('show_airport'):
    # Randomly generate one place
    initOK, okStatus, query_string = generateRandomString (1)
    if initOK == False:
      errorMsg = 'Error: The OpenTrepLibrary cannot be initialised'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    elif okStatus == False or query_string == '':
      errorMsg = 'Error: The OpenTrepLibrary cannot generate ' + str(nbOfDraws) + ' places'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    result = True
  elif search_form.has_key('show_itinerary'):
    # Randomly generate three places
    initOK, okStatus, query_string = generateRandomString (3)
    if initOK == False:
      errorMsg = 'Error: The OpenTrepLibrary cannot be initialised'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    elif okStatus == False or query_string == '':
      errorMsg = 'Error: The OpenTrepLibrary cannot generate ' + str(nbOfDraws) + ' places'
      return render (request, 'search/500.html', {'error_msg': errorMsg})
    result = True

  # Detect the Google Map parameters (if any)
  if search_form.has_key('z'):
    zoom_level = search_form['z']
  if search_form.has_key('mt'):
    map_type_value = search_form['mt']

  #
  return query_string, result, place_list, zoom_level, map_type_value

# Calculate the corrected query string, based on the corrected keywords for
# every place
def calculate_corrected_query_string (place_list):
  corrected_query_string = ''
  idx = 0
  for place in place_list:
    if idx != 0:
      corrected_query_string += ' '
    corrected_query_string += ' '.join(place.corrected_keyword_list.word)
    idx += 1

  return corrected_query_string

#
def index (request):
  # Extract the parameters from the incoming request
  query_string, result, place_list, zoom_level, map_type_value = extract_params (request, query_string='')

  if query_string == '':
    #
    return render (request, 'search/index.html', {
        'place_list': None,
        'place_pair_list': [{'dep': None, 'arr': None, 'dist': '0'}],
        'nb_of_places': 0,
        'dist_total': '0',
        'unmatched_keyword_list': None, 'n_unmatched_kw': 0,
        'query_string': query_string,
        'corrected_query_string': query_string,
        'coord_for_GMap_center': '',
        'zoom_level': zoom_level, 'map_type_value': map_type_value})
  else:
    # Delegate the call to the display view
    return display (request, query_string, zoom_level, map_type_value)

#
def display (request, query_string = '',
             zoom_level = 5, map_type_value = 'HYBRID'):
  # Logging
  logger = logging.getLogger ('django.request')
  logger.info (str(request))
  logger.info ('Query string: "' + query_string + '"')

  #
  if query_string == '':
    return HttpResponseRedirect (reverse ('search:index'))
        

  # Initialise the OpenTrep C++ library
  initOK, openTrepLibrary = initOpenTrep()
  if initOK == False:
    errorMsg = 'Error: The OpenTrepLibrary cannot be initialised'
    logger.error (errorMsg)
    return render (request, 'search/500.html', {'error_msg': errorMsg})

  # Call the underlying C++ OpenTREP library. The input string is converted
  # into UTF-8 (from Unicode), so that the OpenTREP library be happy with it.
  query_string_str = smart_bytes (query_string, encoding='utf-8',
                                  strings_only=True, errors='strict')
  result = openTrepLibrary.search ('P', query_string_str)

  # Extract the answer
  okStatus, queryAnswer = extractAnswer (result)

  # Query status
  if okStatus == False:
    errorMsg = 'Error in the OpenTREP library: ' + queryAnswer.error_msg.msg
    logger.error (errorMsg)
    return render (request, 'search/500.html', {'error_msg': errorMsg})

  # List of places
  placeList = queryAnswer.place_list

  # List of unmatched keywords
  unmatchedKeywordList = queryAnswer.unmatched_keyword_list

  #
  interpretedString, msg = '', ''
  coord_for_GMap_center = {'lat': 10, 'lon': 0}

  # DEBUG
  logger.debug ('Result: ' + str(placeList))

  # Free the OpenTREP library resource
  openTrepLibrary.finalize()

  # Calling appropriate program (nothing, coordinates, or distance)
  place_list = placeList.place
  n_airports = len (place_list)
  place_pair_list = []
  dist_total = 0
  unmatched_keyword_list = unmatchedKeywordList.word
  n_unmatched_kw = len (unmatched_keyword_list)

  ##
  # No airport (=> no map)
  ##
  if n_airports == 0:
    if (result == True):
      msg += 'Your entry ("%s") was not recognised:<br>' % query_string_str
      msg += 'Enter a three-letter code or the name of an airport or city to find out its coordinates (e.g., JFK, or Rio de Janeiro)<br>'
      msg += 'Enter a sequence of two three-letter place_list or names to find out the distance between those airports (e.g., JFK CDG Tokyo)'

  ##
  # A single airport => a map with a single marker
  ##
  elif n_airports == 1:
    place = place_list[0]
    place_pair_list = [{'dep': place, 'arr': None, 'dist': '0'}]
    coord_for_GMap_center = {'lat': place.coord.latitude,
                             'lon': place.coord.longitude}

  ##
  # Several airports => a map with several paths (and markers)
  ##
  elif n_airports >= 2:
    # Iterate on two lists, one browsing elements [1, N-1],
    # the other browsing the elements [2, N]
    place_list_front = place_list[:-1]
    place_list_second = place_list[1:]
    for (place1, place2) in zip (place_list_front, place_list_second):
      dist = get_distance_km (place1, place2)
      dist_total += dist
      place_pair_list.append ({'dep': place1, 'arr': place2, 
                               'dist': '%d' % dist})

  # Calculate the corrected query string
  corrected_query_string = calculate_corrected_query_string (place_list)

  #
  logger.info ('Query string: "' + query_string
               + '", corrected query string: "' + corrected_query_string
               + '", unmatched keywords: "' + str(unmatched_keyword_list) + '"')
  logger.info ('Place list: ' + text_format.MessageToString (placeList))
  return render (request, 'search/display.html', {
      'place_list': place_list, 'place_pair_list': place_pair_list,
      'nb_of_places': n_airports,
      'dist_total': '%d' % dist_total,
      'unmatched_keyword_list': unmatched_keyword_list,
      'n_unmatched_kw': n_unmatched_kw,
      'query_string': query_string,
      'corrected_query_string': corrected_query_string,
      'coord_for_GMap_center': coord_for_GMap_center,
      'zoom_level': zoom_level, 'map_type_value': map_type_value})

