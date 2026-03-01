
#macro APTABASE_APP_VERSION     VERSION

#macro APTABASE_EU_HOST         "https://eu.aptabase.com"
#macro APTABASE_US_HOST         "https://us.aptabase.com"

// Self-hosted host for A-SH-* keys.
#macro APTABASE_SH_HOST         ""

// Base URL for API requests. If not set, the SDK will use the default Aptabase cloud host based on the AppKey.
#macro APTABASE_BASE_URL        ""

#macro APTABASE_IS_DEBUG        debug_mode

// Max number of events to send in a single batch request. Maximum value is 25.
#macro APTABASE_MAX_BATCH_SIZE  25

// Time interval (in seconds) for automatic flushing of events.
#macro APTABASE_FLUSH_INTERVAL  10

// Whether to use randomize() on session ID generation.
#macro APTABASE_RANDOMIZE       true
