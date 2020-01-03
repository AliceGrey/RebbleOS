#pragma once
/* event_service.h
 * A Service bus for subscribing to messages of interest
 *   (https://github.com/pebble-dev)
 * RebbleOS
 * 
 * Author: Barry Carter <barry.carter@gmail.com>.
 */

typedef enum EventServiceCommand {
    EventServiceCommandGeneric,
    EventServiceCommandCall,
    EventServiceCommandMusic,
    EventServiceCommandNotification,
} EventServiceCommand;


/**
 * @brief Prototype for the callback on window creation
 * 
 * @param overlay An \ref OverlayWindow pointer with the newly created window
 */
typedef void (*EventServiceProc)(EventServiceCommand command, void *data);

typedef void (*MusicServiceProc)(MusicTrackInfo *music);
typedef void (*NotificationServiceProc)(Uuid *uuid);
typedef void (*DestroyEventProc)(void *data);


/** 
 * @brief Subscribe to a given \ref EventServiceCommand
 * 
 * @param command \ref EventServiceCommand to get events for
 * @param callback callback to the function to be invoked on reception of the \ref EventServiceCommand
 */
void event_service_subscribe(EventServiceCommand command, EventServiceProc callback);

/** 
 * @brief Un-Subscribe from a given \ref EventServiceCommand
 * 
 * @param command \ref EventServiceCommand to stop watching
 */
void event_service_unsubscribe(EventServiceCommand command);

/** 
 * @brief Un-Subscribe a given thread from all watched events
 */
void event_service_unsubscribe_all(void);

/** 
 * @brief As a client application, post a \ref EventServiceCommand data and the destroyer of said data.
 * Data is proxied through the relevant thread(s) to find a recipient of the request
 * 
 * @param command \ref EventServiceCommand to stop watching
 * @param data \ref payload data to send as part of the event. It is expected the recipient knows what to do with it
 * @param destroy_callback a callback to the method that allows the event to destroy any data it might need to
 */
void event_service_post(EventServiceCommand command, void *data, DestroyEventProc destroy_callback);

/** 
 * @brief Called from a specific thread to process the data.
 * Internal.
 * 
 * @param command \ref EventServiceCommand to stop watching
 * @param data \ref payload data to send as part of the event. It is expected the recipient knows what to do with it
 * @param destroy_callback a callback to the method that allows the event to destroy any data it might need to
 */
void event_service_event_trigger(EventServiceCommand command, void *data, DestroyEventProc destroy_callback);
