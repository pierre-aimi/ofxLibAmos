/**
* @file amos.h
* @brief The libAMOS API.
* @details libAMOS is a C++ library implementing the Aimi Musical Operating System (AMOS). 
* It comprises a scriptable audio engine, generative composition environment, and connectivity
* to a cloud based music ecosystem for playback, creation and publishing of generative AI powered 
* musical experiences. Functionality is exposed to clients through a C-interface to allow the
* library to be imported through Foreign Function Interfaces in a large number of languages.
*
* The library is a singleton which needs to be created (via amos_create) before it is used. 
* All subsequent calls of API functions must be made on the same thread as amos_create was 
* called.  Exceptions to this are amos_get_user_fader_value and amos_ramp_user_fader, which
* can be called from another thread (although that thread should be consistent). 
*
* A number of these API functions are for requesting information from libAMOS, for example
* a list of all experiences, details such as download stats about a particular experience etc.
* Most of these requests have synchronous and asynchronous versions.  The sychronous version
* returns the requested information as a JSON object in a c-style string (const char*). The
* asynchronous version returns void immediately, and the requested data is asynchronously
* posted via the clients nominated callback function, which the client sets using the API call
* amos_set_msg_object_callback.  Note that the asynchronous versions still need to be called
* on the same thread as amos_create - however the client provided callback will be executed
* on an aimiscript process thread. It is recommended (but not required) that clients callbacks
* dispatch further handling of these messages to a separate thread, since heavy computation
* on an aimiscript process thread may slow down delivery of subsequent messages.
*
* Mobile clients typically consume libamos compiled with the WITH_EXTERNAL_SIK flag set.
* This means that libAMOS itself does not communicate directly with audio hardware.
* Rather, it exposes an audioRender function. The client is responsible for pulling audio data 
* from AMOS at the rate required by the audio hardware. AMOS assumes 48kHz sample rate,
* 32-bit floating point stereo audio stream internally. The client is responsible for performing 
* samplerate and format conversions before passing on the audio data to the audio hardware.
*
*
* @author Aimi
* @date 9/12/2022 
*/

#ifdef __cplusplus
// this opens an extern "C" block, but only if being included for a C++ compile
//  if this is being included in a C compile, the extern "C" bits won't be seen
extern "C" {
#endif

/** 
 * @brief AMOS singleton
 * @param workingDir Directory that aimi_rex.db and aimi_script.log will live in. For mobile client this should generally be the default documents directory for the app.
 * @param modulesDir Directory that contains the core aimiscript modules. Mobile clients can ignore this argument (pass NULL) as modules are inlined into the library image.  Other clients will typically use ${workingDir}/modules.
 * @param motherEndpoint URL endpoint for the cloud database (called mother). Player apps should use "https://app.aimi.fm". Creator apps should use "https://studio.aimi.fm"
 * @param postOfficePort Socket port for amos to communicate messages to the app via nng (NextNanoMessage). 5563 is the typical value, however the client should ensure the nominated port is free. Clients can ignore this (pass 0) if the separately call set_amos_msg_callback
 * @param audioSocketPort Socket port for amos to stream master audio to the app via nng (NextNanoMessage). Pass 0 for no audio stream (which is typical).
 * @param logLevel Log level threshold (0 - 5) below which log level messages are ignored.
 */
int amos_create(const char* workingDir, const char *modulesDir, const char* motherEndpoint, int postOfficePort, int audioSocketPort, int logLevel);


/**
 * @brief Destroy AMOS singleton
 * 
 */
void amos_destroy();


/**
 * @brief Set the amos msg callback function.
 * 
 * If a client prefers to use their own callback for recieving messages, they can set the callback here. If this is not set, then messages default to being sent over nng.
 * 
 * @param object Opaque pointer to an object (objective-c object or swift object) that has a msg handler member function. This will get passed back into the provided callback as its firts argument
 * @param cb C function pointer, taking an opaque pointer to an object as its first argument, and the message itself as its second argument. Clients will generally provide a callback that casts the first argument to an object, then call a member function with the second argument
 */
void amos_set_msg_object_callback(void* object, void(*cb)(void*, const char*));


/**
 * @brief Sets the JWT for AMOS REST calls
 * 
 * @param token Java Web Token obtained by client from Auth0, passed on to AMOS to authenticate REST calls with
 */
void amos_set_login_token(const char* token);


/**
 * @brief Sets the role for cloud database login
 * 
 * Generally this is either aimi_admin or aimi_user. This isnt needed for mobile player apps, it is only needed for creator apps that need to edit data in the cloud.
 * Note that just setting the role to aimi_admin here does not automatically grant admin rights to the user. The role needs to match that which is embedded in the JWT
 * and the user must actually have that role already in the cloud.
 * 
 * @param token login role
 */
void amos_set_login_role(const char* role);


/**
 * @brief The password for decrypting audio content stored in the local database
 * 
 * @param pw 
 */
void amos_set_decryption_pw(const char* pw);


/**
 * @brief Sets the email (i.e. username) for direct database login
 * 
 * This is for direct datbase user login only, generally for CI build pipeline test suite apps
 * @param email the email of the direct database user (which serves as the username)
 */
void amos_set_direct_login_email(const char* email);


/**
 * @brief Sets the password for direct database login
 * 
 * This is for direct database user login only, generally for CI build pipeline test suite apps
 * @param token direct login password
 */
void amos_set_direct_login_pw(const char* pw);


/**
 * @brief Login for a direct database user
 * 
 * This is for direct database user login only, generally for CI build pipeline test suite apps
 * Returns the http response code. i.e. a value of 200 means the operation was successful, while a 403 means access was denied
 */
int amos_direct_login();


/**
 * @brief Download the list of experiences.
 * 
 * Should be called AFTER amos_set_login_token. Downloads from the cloud database (mother) the list of experiences available to the client, and stores in the local database (daughter).
 * This essentially makes a local cache of the experience table. This includes top-level metadata about the experience such as the title, 
 * artist, image_url etc.  It does not include detailed metadata such as the themes belonging to the experience.  The downloaded data is only sufficient
 * to display a list of available experiences, not to play back an experience.  For detailed metadata, call amos_cache_experience_metadata(experience_id).
 * 
 * Under the hood, this downloads the list of experiences on the aimiscript download process (procnum 1).
 * When downloading is complete, or the function times-out, a notification is posted with body 
 * { tags: ['download', 'experiences'], request: requestid, result: res } where res is a boolean indicating success. 
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_cache_experience_list(long requestid);


/**
 * @brief Download the list of artists.
 * 
 * Should be called AFTER amos_set_login_token. Downloads from the cloud database (mother) the list of artists and stores in the local database (daughter).
 * This essentially makes a local cache of the artist table. 
 * 
 * Under the hood, this downloads the list of artist on the aimiscript download process (procnum 1).
 * When downloading is complete, or the function times-out, a notification is posted with body 
 * { tags: ['download', 'artists'], request: requestid, result: res } where res is a boolean indicating success. 
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_cache_artist_list(long requestid);


/**
 * @brief Download metadata for an experience
 * 
 * Should be called AFTER amos_set_login_token, and BEFORE attempting to playback an experience. 
 * Downloads detailed metadata for an experience, including all the themes referenced by the experience, and their corresponding file/media table rows.
 * This does not download the actual audio/midi blobs, just the metadata. However this metadata is sufficient to start playback of the experience (which)
 * will automatically handle downloading of the audio/midi blobs itself
 *
 * Under the hood, this downloads the experience metadata on the aimiscript download process (procnum 1).
 * When downloading is complete, or the function times-out, a notification is posted with body 
 * { tags: ['download', 'metadata'], request: requestid, experienceId: experienceid, result: res } where res is a boolean indicating success. 
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param experienceid ID of the experience to be downloaded (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 */
void amos_cache_experience_metadata(long requestid, long experienceid);


/**
 * @brief Cue up an experience for playing next.
 * 
 * If nothing else is playing this should start playback of the given experience, otherwise will initiate a transition. Eventually this
 * transition should be smooth, however for the moment it may simply stop the playing experience and then start the cued experience.
 * 
 * @param experienceid ID of the experience to be played (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 */
void amos_cue_playback(long experienceid);


/**
 * @brief Retrieve list of experiences available to the user
 * 
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return all currently cached experiences.
 *
 * @return JSON representation of list of experiences as a string. Client should call amos_free on this return value when finished with it.
 */
const char* amos_experiences_get_all(bool force);


/**
 * @brief Retrieve list of experiences available to the user asynchronously
 * 
 * Client should receive JSON representation of list of experiences as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return all currently cached experiences.
 */
void amos_experiences_get_all_async(long requestid, bool force);


/**
 * @brief Retrieve detailed metadata about an experience
 * 
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return the currently cached version of this experience.
 * 
 * @return JSON representation of the experience as a string. Client should call amos_free on this return value when finished with it.
 */
const char* amos_experiences_get(long experienceid, bool force);


/**
 * @brief Retrieve detailed metadata about an experience asynchronously
 * 
 * Client should receive JSON representation of the experience as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return the currently cached version of this experience.
 */
void amos_experiences_get_async(long requestid, long experienceid, bool force);


/**
 * @brief Calculate the number of themes used in an experience
 * 
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * @returns A number representing the number of themes, or 0 if the experience was not found
 */
const char* amos_experiences_get_theme_count(long experienceid);


/**
 * @brief Calculate the number of themes used in an experience and report asynchronously
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * 
 * Client should receive JSON representation of the results as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 */
void amos_experiences_get_theme_count_async(long requestid, long experienceid);


/**
 * @brief Calculates the number of plays for an experience. Currently we only store play counts for the current and previous month.
 * 
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * @returns A number representing the number of plays of themes in this experience, or 0 if the experience was not found
 */
const char* amos_experiences_get_play_count(long experienceid);


/**
 * @brief Calculates the number of plays for an experience and report asynchronously. Currently we only store play counts for the current and previous month.
 *
 * Client should receive JSON representation of the results as a c-string via their nominated callback (as set in amos_set_msg_object_callback) 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 */
void amos_experiences_get_play_count_async(long requestid, long experienceid);


/**
 * @brief Retrieves metadata for all artists the current user has access to.
 *
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return all currently cached artists.
 *
 * @return JSON representation of list of artists as a string. Client should call amos_free on this return value when finished with it.
 */
const char* amos_artists_get_all(bool force);


/**
 * @brief Retrieve list of artists the current user has access to asynchronously
 * 
 * Client should receive JSON representation of list of artists as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return all currently cached artists.
 */
void amos_artists_get_all_async(long requestid, bool force);


/**
 * @brief Retrieve detailed metadata about an artist
 *
 * @param artistid ID of the artist to be retrieved (i.e. the column called id in the mother artist table)
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return the currently cached version of this artist.
 *
 * @return JSON representation of the artist as a string. Client should call amos_free on this return value when finished with it.
 */
const char* amos_artists_get(long artistid, bool force);


/**
 * @brief Retrieve detailed metadata about an artist asynchronously
 *
 * Client should receive JSON representation of the artist as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 *
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param artistid ID of the artist to be retrieved (i.e. the column called id in the mother artist table)
 * @param force Whether to force a refresh from mother. If false, the api will prefer to return the currently cached version of this artist.
 */
void amos_artists_get_async(long requestid, long artistid, bool force);


/**
 * @brief Performs garbage collection on the local sqlite database
 *
 * This will free up space from unloaded experiences. This is quite a time-consuming task, which also locks the database, so should be performed when the app is not active.
 * 
 */
void amos_tasks_clean_db();


/**
 * @brief Release the resources associated with a returned string from amos.
 * 
 * Any libamos API functions that return a C-Style string (i.e. a character pointer) need to be freed by the client once the client has finished with it.
 * 
 * @param ptr C-style string returned from amos
 */
void amos_free(void* ptr);


/**
 * @brief Retrieve breakdown of memory usage per experience
 * 
 * Client should receive JSON representation of the memory usage list as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 */
const char* amos_get_disk_usage();


/**
 * @brief Retrieve breakdown of memory usage per experience asynchronously
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @return JSON representation of the artist as a string. Client should call amos_free on this return value when finished with it.
 */
void amos_get_disk_usage_async(long requestid);


/**
 * @brief Delete the audio content for the nominated experience from the local daughter database.
 * 
 * Note that storage will not actually be reclaimed until the sqlite3 database is vacuumed, such as with amos_tasks_clean_db()
 * 
 * @param experienceid ID of the experience to be retrieved (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 */
void amos_unload_experience(long experienceid);


/**
 * @brief Retrieve current value for the 'user fader' on the nominated track
 *
 * AMOS provides an additional 'user fader' on each track to allow for interactive apps to modify the volume of that track interactively
 * and independently of the score (which is controlling a separate fader on the same track).
 *  
 * @param track index of the track (aka group) to get the fader value for. 0 = Beats, 1 = Bass, 2 = Harmony, 3 = Pads, 4 = Tops, 5 = Melody, 6 = FX
 * 
 * @return The current value of the user fader for that track.
 */
double amos_get_user_fader_value(int track);


/**
 * @brief Set the value for the 'user fader' on the nominated track over a period of time (linear ramp)
 *
 * AMOS provides an additional 'user fader' on each track to allow for interactive apps to modify the volume of that track interactively
 * and independently of the score (which is controlling a separate fader on the same track). This function allows the app to change the value
 * from where it is currently to the target value, with a linear ramp over a duration measured in beats.
 *  
 * NOTE - subsequent attempts to set the fader will wait until any previous ramps are completed, then rush to the new target value
 * in time for the desired end-beat if possible, or straight away if not.
 * 
 * @param track index of the track (aka group) to get the fader value for. 0 = Beats, 1 = Bass, 2 = Harmony, 3 = Pads, 4 = Tops, 5 = Melody, 6 = FX
 * @param target_value the value the fader should end up at
 * @param duration length in beats over which to ramp the fader from its current value to the target value
 * 
 */
void amos_ramp_user_fader(int track, double target_value, double duration);


/**
 * @brief Shuffle themes.
 * .
 * Shuffle "playing" themes chooses a suitable random file to replace the currently playing file.
 * amos_shuffle shuffles tracks whose bit is correctly set in group.
 * 
 * Toggle on and off bits for the 7 groups - remember this has to be "C" friendly on the other end
 * 
 * @param groups is a bitset for the 7 groups
 */
void amos_shuffle(const unsigned char groups);

/**
 * @brief Shuffle all themes.
 * .
 * A convenience function that shuffles ALL "playing" themes
 *   
 */
void amos_shuffle_all();


/**
 * @brief Retrieve list of macro 'sliders' supported by the current score (if any) asynchronously
 * .
 * Aimi Scores may present a 'slider' capability, which means that the score has defined 
 * some macro parameter controls that provide musical functionality for a client to easily manipulate.
 * This function returns a JSON array of sliders as reported by the score, or an empty array if 
 * the score doesn't have this capability. The elements of this array are JSON objects of the form
 * { id: 1, name: "Display Name", description: "What does it do?", limits: [a, b], temporalScope: "section" }
 * where the id is unique amongst this list, the limits are the numerical values between which the slider
 * can move, and the temporalScope gives a sense of when audible effects are likely to be heard from adjusting
 * this slider.  Possible scopes are immediate, track, section and static. An immediate slider should have 
 * immediately audible results. A track or section slider won't have any audible effect until the start of the
 * next track/section (and client apps can anticipate how long it will be until this happens by following the
 * various score and transport messages).  So for example a client app may wish to visually indicate somehow
 * the time until a section slider can be expected to come into effect. Note that the stochastic nature of
 * scores means that some slider changes may not be obvious even when they have 'come into effect'.
 * 
 * Note that, because this information in only available in the aimiscript PLAY process, this function only
 * is only available in async format
 * 
 * Client should receive JSON object of the form
 * { tags: ['score', 'slider', 'list'], request: requestid, result: [{ id: 1, name: "abc", description: "xyz", limits: [a, b], temporalScope: "section" }, ...] }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_get_score_sliders_async(long requestid);


/**
 * @brief Get the current value of a slider by id
 * .
 * Aimi Scores may present a 'slider' capability, which means that the score has defined 
 * some macro parameter controls that provide musical functionality for a client to easily manipulate.
 * This function reports the current value of the slider with given id, if it exists (or zero otherwise)
 * Because this function retrieves data from the aimiscript PLAY process, it is only supplied in async form
 * The asynchronous nature of the call means it is possible that rapid successive calls to this function
 * may result in posted notifications being out-of-order.  For this reason the time at which the slider 
 * value was taken is included in the return object. The absolute value of this time is probably less interesting
 * to the client than the relative value - for example to filter out older results from the stream.
 * 
 * Client should receive JSON object of the form
 * { tags: ['score', 'slider', 'value'], request: requestid, result: { id: 1, time: t, value:val } }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_get_score_slider_value_async(long requestid, long id);


/**
 * @brief Set the current value of a slider by id
 * .
 * Aimi Scores may present a 'slider' capability, which means that the score has defined 
 * some macro parameter controls that provide musical functionality for a client to easily manipulate.
 * This function sets the value of the slider with given id, if it exists
 */
void amos_set_score_slider_value(long id, double value);


/**
 * @brief Register a thumbs-up event for a given group (aka track)
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, which may be applied to a given group.
 * Calling this function will delegate to the score provided thumbsUpOnTrack(trackNum) function if it exists, otherwise is a no-op
 * 
 * @param trackNum index of the group 
 */
void amos_score_thumbs_up_on_track(int trackNum);


/**
 * @brief Register a thumbs-down event for a given group (aka track)
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, which may be applied to a given group.
 * Calling this function will delegate to the score provided thumbsDownOnTrack(trackNum) function if it
 * exists, otherwise is a no-op
 * 
 * @param trackNum index of the group 
 */
void amos_score_thumbs_down_on_track(int trackNum);


/**
 * @brief Register a thumbs-up event for the 'master'
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for the experience as a whole.
 * Calling this function will delegate to the score provided thumbsUp function if it
 * exists, otherwise is a no-op.
 */
void amos_score_thumbs_up();


/**
 * @brief Register a thumbs-down event for the 'master'
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for the experience as a whole.
 * Calling this function will delegate to the score provided thumbsDown() function if it
 * exists, otherwise is a no-op.
 * 
 * @param trackNum index of the group 
 */
void amos_score_thumbs_down();


/**
 * @brief Return a JSON array of currently playing themes on each group asynchronously
 * Corresponds to groups 0 = Beats, 1 = Bass, 2 = Harmony, 3 = Pads, 4 = Tops, 5 = Melody, 6 = FX
 * This is only available asynchronously since it queries the PLAY aimiscript process
 * 
 * Client should receive a JSON object of the form
 * { tags: ['response', 'playing', 'themes'], request: requestid, result: [ id0 id1 id2 id3 id4 id5 id6 ] },
 * where each id is either a themeid or null,
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * 
 */
void amos_score_currently_playing_themes_async(long requestid);


/**
 * @brief Return a string of the identifier for the currently playing section asynchronously
 * This is only available asynchronously since it queries the PLAY aimiscript process
 * 
 * Client should receive a JSON object of the form
 * { tags: ['response', 'playing', 'section'], request: requestid, result: sectionKey },
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * 
 */
void amos_score_currently_playing_section_async(long requestid);


/**
 * @brief Return a string of the identifier for the currently playing experience asynchronously
 * This is only available asynchronously since it queries the PLAY aimiscript process
 * 
 * Client should receive a JSON object of the form
 * { tags: ['response', 'playing', 'experience'], request: requestid, result: experienceID },
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * 
 */
void amos_score_currently_playing_experience_async(long requestid);


/**
 * @brief Setup system sliders
 * .
 * If a client wishes to use the system provided sliders, they should call this function first.
 * This sets various audio parameters into the correct state to be controlled by the system sliders.
 * If the client also wishes to control score/and audio parameters directly, it may need to 
 * later call this again when switching back to system slider control
 */
void amos_setup_system_sliders();


/**
 * @brief Retrieve list of macro 'sliders' supported by the system asynchronously
 * .
 * Aimi provides some 'universal' macro sliders, such as 'progression' and 'intensity', 
 * that provide musical functionality for a client to easily manipulate.
 * This function returns a JSON array of sliders as reported by the system. 
 * The elements of this array are JSON objects of the form
 * { name: "identifier", limits: [a, b] }
 * where the name is a string identifier (typically lowercase such as 'intensity').  These strings are 
 * fixed (currently 'progression', 'intensity', 'texture', 'vocals') and client apps need to set their own
 * display names with localisation etc. The limits are the numerical values between which the slider can move.
 * 
 * Note that scores may override the system provided functionality, however this is handled under the hood.
 * 
 * Note that, because this information in only available in the aimiscript PLAY process, this function only
 * is only available in async format
 * 
 * Client should receive JSON object of the form
 * { tags: ['system', 'slider', 'list'], request: requestid, result: [{ name: "abc", limits: [a, b] }, ...] }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_get_system_sliders_async(long requestid);


/**
 * @brief Get the current value of a slider by name
 * .
 * Aimi provides some 'universal' macro sliders, such as 'progression' and 'intensity', 
 * that provide musical functionality for a client to easily manipulate.
 * This function reports the current value of the slider with given name, if it exists (or zero otherwise)
 * Because this function retrieves data from the aimiscript PLAY process, it is only supplied in async form
 * The asynchronous nature of the call means it is possible that rapid successive calls to this function
 * may result in posted notifications being out-of-order.  For this reason the time at which the slider 
 * value was taken is included in the return object. The absolute value of this time is probably less interesting
 * to the client than the relative value - for example to filter out older results from the stream.
 * 
 * Client should receive JSON object of the form
 * { tags: ['system', 'slider', 'value'], request: requestid, result: { name: 'progression', time: t, value:val } }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_get_system_slider_value_async(long requestid, const char* name);


/**
 * @brief Set the current value of a system provided slider by name
 * .
 * Aimi provides some 'universal' macro sliders, such as 'progression' and 'intensity', 
 * that provide musical functionality for a client to easily manipulate.
 * This function sets the value of the slider with given name, if it exists
 */
void amos_set_system_slider_value(const char* name, double value);


/**
 * @brief Register a thumbs-up event for the 'master'
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for the experience as a whole.
 * Calling this function will call the system provided thumbsUp function.
 */
void amos_system_thumbs_up();


/**
 * @brief Register a thumbs-down event for the 'master'
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for the experience as a whole.
 * Calling this function will call the system provided thumbsDown() function.
 * 
 * @param trackNum index of the group 
 */
void amos_system_thumbs_down();


/**
 * @brief Register a system thumbs-up event for the track
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for a particular track.
 * Calling this function will call the system provided thumbsUp function.
 * 
 * @param track_num index of the track 
 */
void amos_system_thumbs_up_on_track(int track_num);


/**
 * @brief Register a thumbs-down event for the track
 * 
 * User interfaces may provide thumbs-up and thumbs-down buttons, for a particular track.
 * Calling this function will call the system provided thumbsDown() function.
 * 
 * @param track_num index of the track 
 */
void amos_system_thumbs_down_on_track(int track_num);


/**
 * @brief Report whether metadata is available for a given experience.
 * .
 * Before an experience can be played, its metadata needs to have been downloaded to the daughter database
 * using amos_cache_experience_metadata. For the purposes of offline playback, it is useful to know whether
 * metadata is currently downloaded.
 * 
 * @param expid experienceid ID of the experience to be downloaded (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table)
 * 
 * @returns a boolean indicating if metadata is cached for the given experience *   
 */
bool amos_metadata_is_cached(long expid);


/**
 * @brief Report on themes belonging to a given experience, and their download status
 * .
 * This reports the themes belonging to the given experience according to the cached metadata in daughter. Note that
 * if no metadata has been cached for this experience it would report no themes. The returned value is an object of the
 * number of themes, and the number of themes downloaded.
 *
 * @param expid experienceid ID of the given experience (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table) *
 * 
 * @returns JSON object { themeCount: a, downloadedThemeCount: b }
 */
const char* amos_local_theme_count(long expid);


/**
 * @brief Report on themes belonging to a given experience, and their download status asynchronously
 * .
 * This reports the themes belonging to the given experience according to the cached metadata in daughter. Note that
 * if no metadata has been cached for this experience it would report no themes. The returned value is an object of the
 * number of themes, and the number of themes downloaded.
 * 
 * Client should receive JSON object 
 * { tags: ['response', 'experience', 'local_theme_count'], request: requestid, result: { themeCount: a, downloadedThemeCount: b }}
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param expid experienceid ID of the given experience (i.e. the column called id in the mother experience table, or the column called experienceid in the daughter experience table) *
 */
void amos_local_theme_count_async(long requestid, long expid);


/**
 * @brief Report on themes belonging to each experience, and their download status
 * .
 * This reports the themes belonging to each experience according to the cached metadata in daughter. Note that
 * if no metadata has been cached for an experience it would report no themes. The returned value is an array of objects of the
 * experience ID, number of themes, and the number of themes downloaded. This function may be more efficient than looping
 * over the experiences and calling amos_local_theme_count, since it gathers all the information in a single db query
 *
 * @returns JSON array [{ experienceId: expid, themeCount: a, downloadedThemeCount: b }, ...] as a c-string
 */
const char* amos_local_theme_counts();


/**
 * @brief Report on themes belonging to each experience, and their download status asynchronously
 * .
 * This reports the themes belonging to each experience according to the cached metadata in daughter. Note that
 * if no metadata has been cached for an experience it would report no themes. The returned value is an array of objects of the
 * experience ID, number of themes, and the number of themes downloaded. This function may be more efficient than looping
 * over the experiences and calling amos_local_theme_count, since it gathers all the information in a single db query
 *
 * Client should receive JSON object { tags: ['response', 'experience', 'local_theme_counts'], request: requestid, result: [{ experienceId: expid, themeCount: a, downloadedThemeCount: b }, ...] }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback)
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 *
 */
void amos_local_theme_counts_async(long requestid);


/**
 * @brief Start sending beat messages
 * .
 * Starts the flow of musical transport messages. These messages contain the current global musical beat, and the current tempo.
 * Client should receive JSON object 
 * { tags: ['beat', 'transport'], result: { beat: b, time: t, seconds: s, frame: f, tempo: bpm } }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback).
 * 
 * @param beat_period The subdivision or multiple of beat at which to send messages. For example 0.25 means send every 16th note.
 */
void amos_start_transport_msgs(double beat_period);


/**
 * @brief Stop sending beat messages
 * .
 * Stops the flow of musical transport messages (as described above). Note that in order to change the period of the beat
 * messages one should first stop the messages, then start them again with the new desired beat period.
 */
void amos_stop_transport_msgs();


/**
 * @brief Start sending per-group Root-Mean-Square volume messages
 * .
 * Starts the flow of Root-Mean-Square volume (RMS) messages. These messages contain the current audio volume for each group.
 * Client should receive JSON object 
 * { tags: {tags: ['rms', 'logger'], beat: b, 0:rms0, 1:rms1, 2:rms2, 3:rms3, 4:rms4, 5:rms5, 6:rms6 }
 * as a c-string via their nominated callback (as set in amos_set_msg_object_callback). The RMS values
 * correspond to the groups in the order ['Beats', 'Bass', 'Harmony', 'Pads', 'Tops', 'Melody', 'FX']
 * 
 * @param beat_period The subdivision or multiple of beat at which to send messages. For example 0.25 means send every 16th note.
 */
void amos_start_rms_msgs(double beat_period);


/**
 * @brief Stop sending Root-Mean-Square volume messages
 * .
 * Stops the flow of rms messages (as described above). Note that in order to change the period of the beat
 * messages one should first stop the messages, then start them again with the new desired beat period.
 */
void amos_stop_rms_msgs();


/**
 * @brief Override choice of next musical section
 * .
 * The section_key should come from the list of sections in the score, probably from receiving a 
 * ['player', 'section', 'matrix'] message.  It will generally work best if the selected next_section
 * is on the list of possible next sections following the current section according to the section matrix
 * 
 * @param section_key The identifier for the desired section as a c-string
 */
void amos_override_next_section(const char* section_key);


/**
 * @brief Download user preferences from backend cloud db and store in local db
 * .
 * This requires the user to have logged in. It will store the downloaded preferences object in the
 * user_preferences table in a row with the users uuid. If there is an existing user preferences object
 * in the local db, this will perform a deep merge, favouring the local data on conflict.
 * 
 * @return true if successful, false if otherwise
 */
bool amos_download_user_preferences();


/**
 * @brief Download user preferences from backend cloud db and store in local db on the download process
 * .
 * This requires the user to have logged in. It will store the downloaded preferences object in the
 * user_preferences table in a row with the users uuid. If there is an existing user preferences object
 * in the local db, this will perform a deep merge, favouring the local data on conflict.
 * 
 * Under the hood, this downloads the user preferences on the aimiscript download process (procnum 1).
 * When downloading is complete, or the function times-out, a notification is posted with body 
 * { tags: ['download', 'user_preferences'], request: requestid, result: res } where res is a boolean indicating success. 
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_download_user_preferences_async(long requestid);


/**
 * @brief Upload user preferences from local db to cloud backend db
 * .
 * This requires the user to have logged in. If there is an existing user preferences object
 * in the cloud db, this will perform a deep merge, favouring the local data on conflict.
 * 
 * @return true if successful, false if otherwise
 */
bool amos_upload_user_preferences();


/**
 * @brief Upload user preferences from local db to cloud backend db asynchronously
 * .
 * This requires the user to have logged in. If there is an existing user preferences object
 * in the cloud db, this will perform a deep merge, favouring the local data on conflict.
 * 
 * Under the hood, this uploads the user preferences on the aimiscript download process (procnum 1).
 * When downloading is complete, or the function times-out, a notification is posted with body 
 * { tags: ['download', 'user_preferences'], request: requestid, result: res } where res is a boolean indicating success. 
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 */
void amos_upload_user_preferences_async(long requestid);


/**
 * @brief Retrieve a user preference by keyPath
 * .
 * This expects a JSON-style property path as a period separated string. For example
 * to retrieve the user preference with path ['experiences', 228, 'theme_weights']
 * use keyPath "experiences.228.theme_weights"
 * 
 * @param key_path Period separated path of keys
 * 
 * @return JSON representation of user_preference at the given key_path as a c-string, or null in not found
 */
const char* amos_get_user_preference(const char* key_path);


/**
 * @brief Retrieve a user preference by keyPath asynchronously
 * .
 * This expects a JSON-style property path as a period separated string. For example
 * to retrieve the user preference with path ['experiences', 228, 'theme_weights']
 * use keyPath "experiences.228.theme_weights"
 * 
 * @param requestid Client supplied identifier for this request, which will be returned in the completion notification
 * @param key_path Period separated path of keys
 * 
 * clients should receive a JSON object of the form of
 * { tags: ['response', 'user_preference'], request: requestid, result: preferenceObj }
 * via their nominated callback (as set in amos_set_msg_object_callback)
 */
void amos_get_user_preference_async(long requestid, const char* key_path);


/**
 * @brief Set a user preference by keyPath
 * .
 * This expects a JSON-style property path as a period separated string. For example
 * to retrieve the user preference with path ['experiences', 228, 'theme_weights']
 * use keyPath "experiences.228.theme_weights"
 * 
 * @param key_path Period separated path of keys
 * @param value JSON representation of the user_preference as a c-string
 * 
 * @return boolean indicating success
 */
bool amos_set_user_preference(const char* key_path, const char* json_value);


/**
 * @brief Clear a user preference by keyPath
 * .
 * This expects a JSON-style property path as a period separated string. For example
 * to retrieve the user preference with path ['experiences', 228, 'theme_weights']
 * use keyPath "experiences.228.theme_weights"
 * 
 * @param key_path Period separated path of keys
 * 
 */
void amos_clear_user_preference(const char* key_path);

/**
 * @brief Returns a JSON description of the amos audio parameters
 * 
 * Returns a "dictionary" of audio parameters with all of the parameters
 * names, details (targets, groups, target_idx etc..) ranges (min,max) and default values.
 * see params.js for details
 * 
 * @return JSON string
 * 
 */
const char* amos_audio_parameters_info();

#ifdef WITH_EXTERNAL_SINK
/**
 * @brief Request the next audio buffer from AMOS
 * 
 * The client is responsible for pulling audio data from AMOS at the rate required by the audio hardware.
 * AMOS assumes 48kHz sample rate, 32-bit floating point stereo audio stream internally. The client is responsible
 * for performing samplerate and format conversions before passing on the audio data to the audio hardware
 * 
 * @param buf A floating point audio data buffer of 2 * frame_count samples
 * @param frame_count The number of frames in the buffer
 * @return int 0 on successful processing, otherwise positive return codes are errors
 */
int audioRender(float* buf, unsigned int frame_count);
#endif


/**
 * @brief Retrieve the current filename of the active logger.
 * 
 * @return C-string filename.
 */
const char* amos_get_current_log_filename();


/**
 * @brief Log a string to the aimiscript log file
 * 
 * Writes a string to the aimiscript log file, so long as the application loglevel is less than or equal
 * to the specified message loglevel.
 * 
 * @param logStr a c-string to write to the log file
 * @param the loglevel for this message (0 = DEBUG, 1 = INFO, 2 = DEFAULT, 3 = WARN, 4 = ERROR, 5 = FAULT)
 */
void amos_log_to_logfile(const char* logStr, int loglevel);

#ifdef __cplusplus
void set_amos_msg_lambda_capture_callback(std::function <void(const char* msg)> cppcb);
#endif

/**
 * @brief amos message queue listener callback
 * 
 * @param cppcb 
 */
#ifdef __cplusplus
bool amos_msg_queue_listener(const char* qname, std::function<bool(const char* json)> listener);
#else
bool amos_msg_queue_listener(const char* qname, bool(*listener)(const char* json));
#endif

#ifndef SANDBOX_AIMISCRIPT

// If not sandboxed then allow some direct control
float amos_get_param_value(int ttype, int scope, int target, int target_index, int param_id);
int amos_set_param_value(int ttype, int scope, int target, int target_index, int param_id, float value);
float amos_get_user_param_value(int ttype, int scope, int target, int target_index, int param_id);
int amos_set_user_param_value(int ttype, int scope, int target, int target_index, int param_id, float value);
int amos_get_param_composite_type(int ttype, int scope, int target, int target_index, int param_id);
int amos_set_param_composite_type(int ttype, int scope, int target, int target_index, int param_id, int value);
double amos_get_beat();
char* amos_eval(const char* expr, int procNum);
#endif

#ifdef __cplusplus
// close the extern "C" block, but only if we started it in the first place
}
#endif