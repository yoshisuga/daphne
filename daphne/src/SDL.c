/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "./SDL_internal.h"

#if defined(__WIN32__)
#include "core/windows/SDL_windows.h"
#endif

/* Initialization code for SDL */

#include "SDL.h"
#include "SDL_bits.h"
#include "SDL_revision.h"
#include "SDL_assert_c.h"
#include "events/SDL_events_c.h"
#include "haptic/SDL_haptic_c.h"
#include "joystick/SDL_joystick_c.h"

#include "../main_android.h"

/* Initialization/Cleanup routines */
#if !SDL_TIMERS_DISABLED
extern int SDL_TimerInit(void);
extern void SDL_TimerQuit(void);
extern void SDL_TicksInit(void);
extern void SDL_TicksQuit(void);
#endif


/* The initialized subsystems */
static SDL_bool SDL_bInMainQuit = SDL_FALSE;
static Uint8 SDL_SubsystemRefCount[ 32 ];

/* Private helper to increment a subsystem's ref counter. */
static void
SDL_PrivateSubsystemRefCountIncr(Uint32 subsystem)
{
    int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
    SDL_assert(SDL_SubsystemRefCount[subsystem_index] < 255);
    ++SDL_SubsystemRefCount[subsystem_index];
}

/* Private helper to decrement a subsystem's ref counter. */
static void
SDL_PrivateSubsystemRefCountDecr(Uint32 subsystem)
{
    int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
    if (SDL_SubsystemRefCount[subsystem_index] > 0) {
        --SDL_SubsystemRefCount[subsystem_index];
    }
}

/* Private helper to check if a system needs init. */
static SDL_bool
SDL_PrivateShouldInitSubsystem(Uint32 subsystem)
{
    int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
    SDL_assert(SDL_SubsystemRefCount[subsystem_index] < 255);
    return (SDL_SubsystemRefCount[subsystem_index] == 0);
}

/* Private helper to check if a system needs to be quit. */
static SDL_bool
SDL_PrivateShouldQuitSubsystem(Uint32 subsystem) {
    int subsystem_index = SDL_MostSignificantBitIndex32(subsystem);
    if (SDL_SubsystemRefCount[subsystem_index] == 0) {
      return SDL_FALSE;
    }

    /* If we're in SDL_Quit, we shut down every subsystem, even if refcount
     * isn't zero.
     */
    return SDL_SubsystemRefCount[subsystem_index] == 1 || SDL_bInMainQuit;
}

int
SDL_InitSubSystem(Uint32 flags)
{
    /* Clear the error message */
    SDL_ClearError();

    if ((flags & SDL_INIT_GAMECONTROLLER)) {
        /* game controller implies joystick */
        flags |= SDL_INIT_JOYSTICK;
    }

    if ((flags & (SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))) {
        /* video or joystick implies events */
        flags |= SDL_INIT_EVENTS;
    }

#if !SDL_TIMERS_DISABLED
    SDL_TicksInit();
#endif

    /* Initialize the timer subsystem */
    if ((flags & SDL_INIT_TIMER)){
#if !SDL_TIMERS_DISABLED
        if (SDL_PrivateShouldInitSubsystem(SDL_INIT_TIMER)) {
            if (SDL_TimerInit() < 0) {
                return (-1);
            }
        }
        SDL_PrivateSubsystemRefCountIncr(SDL_INIT_TIMER);
#else
        return SDL_SetError("SDL not built with timer support");
#endif
    }

    /* Initialize the audio subsystem */
    if ((flags & SDL_INIT_AUDIO)){
#if !SDL_AUDIO_DISABLED
        if (SDL_PrivateShouldInitSubsystem(SDL_INIT_AUDIO)) {
            if (SDL_AudioInit(NULL) < 0) {
                return (-1);
            }
        }
        SDL_PrivateSubsystemRefCountIncr(SDL_INIT_AUDIO);
#else
        return SDL_SetError("SDL not built with audio support");
#endif
    }

    return (0);
}

int
SDL_Init(Uint32 flags)
{
    return SDL_InitSubSystem(flags);
}

void
SDL_QuitSubSystem(Uint32 flags)
{
#if !SDL_AUDIO_DISABLED
    if ((flags & SDL_INIT_AUDIO)) {
        if (SDL_PrivateShouldQuitSubsystem(SDL_INIT_AUDIO)) {
            SDL_AudioQuit();
        }
        SDL_PrivateSubsystemRefCountDecr(SDL_INIT_AUDIO);
    }
#endif

#if !SDL_TIMERS_DISABLED
    if ((flags & SDL_INIT_TIMER)) {
        if (SDL_PrivateShouldQuitSubsystem(SDL_INIT_TIMER)) {
            SDL_TimerQuit();
        }
        SDL_PrivateSubsystemRefCountDecr(SDL_INIT_TIMER);
    }
#endif
}

void
SDL_Quit(void)
{
    SDL_bInMainQuit = SDL_TRUE;

    /* Quit all subsystems */
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);

#if !SDL_TIMERS_DISABLED
    SDL_TicksQuit();
#endif

    SDL_ClearHints();
    SDL_AssertionsQuit();

    /* Now that every subsystem has been quit, we reset the subsystem refcount
     * and the list of initialized subsystems.
     */
    memset( SDL_SubsystemRefCount, 0x0, sizeof(SDL_SubsystemRefCount) );

    SDL_bInMainQuit = SDL_FALSE;
}

/* vi: set sts=4 ts=4 sw=4 expandtab: */
