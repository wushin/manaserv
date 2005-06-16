/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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


#ifndef _TMW_DATA_PROVIDER_FACTORY_H_
#define _TMW_DATA_PROVIDER_FACTORY_H_


#include <stdexcept>

#include "dataprovider.h"


namespace tmw
{
namespace dal
{


/**
 * A factory to create data providers.
 */
class DataProviderFactory
{
    public:
        /**
         * Create a new data provider.
         */
        static DataProvider*
        createDataProvider(void)
            throw(std::runtime_error);


    private:
        /**
         * Default constructor.
         */
        DataProviderFactory(void)
            throw();


        /**
         * Destructor.
         */
        ~DataProviderFactory(void)
            throw();


        /**
         * Copy constructor.
         */
        DataProviderFactory(const DataProviderFactory& rhs);


        /**
         * Assignment operator.
         */
        DataProviderFactory&
        operator=(const DataProviderFactory& rhs);
};


} // namespace dal
} // namespace tmw


#endif // _TMW_DATA_PROVIDER_FACTORY_H_