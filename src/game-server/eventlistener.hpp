/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  $Id$
 */

#ifndef _TMWSERV_GAMESERVER_EVENTLISTENER_
#define _TMWSERV_GAMESERVER_EVENTLISTENER_

class Thing;
class Being;
class Character;

struct EventDispatch;

/**
 * Pointer to a dispatch table.
 */
struct EventListener
{
    EventDispatch const *dispatch;
    EventListener(EventDispatch const *d): dispatch(d) {}
};

/**
 * Dispatch table for event notification.
 */
struct EventDispatch
{
    /**
     * Called just after something is inserted in a map.
     */
    void (*inserted)(EventListener const *, Thing *);

    /**
     * Called just before something is removed from a map.
     */
    void (*removed)(EventListener const *, Thing *);

    /**
     * Called just after a being has died.
     */
    void (*died)(EventListener const *, Being *);

    /**
     * Called just before a character is deleted.
     */
    void (*disconnected)(EventListener const *, Character *);

    /**
     * Initializes dispatch methods as missing.
     */
    EventDispatch():
        inserted(0), removed(0), died(0), disconnected(0)
    {}
};

/**
 * Helper for using member functions as dispatch methods. The 3-level structure
 * is due to default template parameter not being allowed on functions yet.
 * Conceptually, this helper takes two parameters: the name of the member
 * variable pointing to the dispatch table and the name of the member function
 * to call on dispatch. With these two parameters, it creates a dispatch
 * method. When called, this free function forwards the call to the member
 * function.
 * Pseudo-syntax for getting a dispatch method:
 * <code>&amp;EventListenerFactory&lt; _, DispatchPointerName &gt;::create&lt; _, MemberFunctionName &gt;::function</code>
 * See the start of the spawnarea.cpp file for a complete example.
 */
template< class T, EventListener T::*D >
struct EventListenerFactory
{
    template< class U, void (T::*F)(U *), class V = U >
    struct create
    {
        static void function(EventListener const *d, V *u)
        {
            /* Get the address of the T object by substracting the offset of D
               from the pointer d. */
            T *t = (T *)((char *)d -
                   ((char *)&(((T *)42)->*D) - (char *)&(*(T *)42)));
            // Then call the method F of this T object.
            (t->*F)(u);
        }
    };
};

#endif