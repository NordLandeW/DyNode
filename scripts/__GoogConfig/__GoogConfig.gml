//The API secret for your property
//This is set up in the admin section of the Google Analytics backend
//https://developers.google.com/analytics/devguides/collection/protocol/ga4/
#macro GOOG_API_SECRET  DyCore_get_goog_api_secret()

//The measurement ID that's generataed for your property
//This can be found in the admin section of the Google Analytics backend
#macro GOOG_MEASUREMENT_ID  DyCore_get_goog_measurement_id()

//Name of the file on disk to store the client ID so that it persists between sessions
//A client ID is automatically generated for the user when this library is run for the first time
//If the cache file cannot be found then a new client ID will be generated
#macro GOOG_PERSISTENT_CACHE  "google_analytics_cache.json"

//Set this to <true> to see more information about what events this library is sending
//This is verbose output and likely not useful in production builds
#macro GOOG_DEBUG  (!DyCore_is_release_build())