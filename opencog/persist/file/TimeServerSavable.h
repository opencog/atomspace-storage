/*
 * opencog/atomspace/TimeServer.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Welter Silva <welter@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_TIME_SERVER_H
#define _OPENCOG_TIME_SERVER_H

#include <set>

#include <opencog/persist/file/SavableRepository.h>
#include <opencog/atomspace/TemporalTable.h>

namespace opencog
{

/**
 * This class is used to associate temporal information (timestamps or timelags) to
 * atom handles. It implements SavableRepository so that it can be saved and loaded by
 * SavingLoading class.
 *
 * TODO: Depending on the use cases, this class would have a better performance
 *       if we use a sortedTemporalList in inverse cronological order.
 */
class TimeServer : public SavableRepository
{

    /**
     * Initializes the TimeServer
     */
    void init();

public:

    static int timeServerEntrys;

    // USED TO SEEK MEMORY LEAK
    //static std::set<Temporal> temporalSet;

    TimeServer();
    virtual ~TimeServer();

    /**
     * Adds into this TimeServer an entry composed by the given Atom Handle and Temporal object.
     */
    void add(Handle, const Temporal&);

    /**
     * Gets a list of HandleTemporalPair objects given an Atom Handle.
     * If the passed Handle object is Handle::UNDEFINED, it matches any Handle.
     * If the optional Temporal object argument is not UNDEFINED_TEMPORAL, it will be used
     * to restrict the return to only HandleTemporalPair objects whose Temporal
     * matches with it, according to the temporal relationship (search criteria) argument to
     * be applied with this given Temporal argument.
     * See the definition of TemporalRelationship enumeration to see the possible values for it.
     *
     * NOTE: The matched entries are appended to a container whose OutputIterator is passed as the first argument.
     *          Example of call to this method, which would return all entries in TimeServer:
     *         std::list<HandleTemporalPair> ret;
     *         timeServer->get(back_inserter(ret), Handle::UNDEFINED);
     */
    template<typename OutputIterator> OutputIterator
    get(OutputIterator outIt, Handle h, const Temporal& t = UNDEFINED_TEMPORAL,
        TemporalTable::TemporalRelationship criterion = TemporalTable::EXACT) const {

        HandleTemporalPairEntry* hte = table->get(h, t, criterion);
        HandleTemporalPairEntry* toRemove = hte;

        while (hte) {
            *(outIt++) = hte->handleTemporalPair;
            hte = hte->next;
        }
        if (toRemove) delete toRemove;

        return outIt;
    }

    /**
     * Removes HandleTemporalPair objects related to a given Atom Handle.
     * If the passed Handle object is Handle::UNDEFINED, it matches any Handle.
     * If the optional Temporal object argument is not UNDEFINED_TEMPORAL, it will be used
     * to restrict the removal to only HandleTemporalPair objects whose Temporal
     * matches with it, according to the temporal relationship (search criteria) argument to
     * be applied with this given Temporal argument.
     * See the definition of TemporalRelationship enumeration to see the possible values for it.
     * @return True if any entry corresponding to the given arguments was removed. False, otherwise.
     */
    bool remove(Handle, const Temporal& = UNDEFINED_TEMPORAL, TemporalTable::TemporalRelationship = TemporalTable::EXACT);

    // Methods from SavableRepository interface:

    /**
     * Returns an identifier for the Repository.
     */
    const char* getId() const;

    /**
     * This method stores this repository in the file specified.
     * @param the file pointer where the TimeServer must be saved.
     */
    void saveRepository(FILE *) const;

    /**
     * This method loads a repository stored in the file specified.
     * @param the file pointer where the TimeServer is stored.
     * @param a map of old Handles (stored in the file) to new Handles (in the current memory).
     */
    void loadRepository(FILE *, HandleMap<Atom *> *);

    /**
     * This method is used to clear the whole TimeServer repository.
     */
    void clear();

    /**
     * Get the timestamp of the more recent upper bound of Temporal object already inserted into this TimeServer.
     */
    unsigned long getLatestTimestamp() const;

private:

    /**
     * The temporal table used by this TimeServer
     */
    TemporalTable* table;

    /**
     * The timestamp of the more recent upper bound of Temporal object already inserted into this TimeServer
     */
    unsigned long latestTimestamp;

    /**
     * Overrides and declares copy constructor and equals operator as private 
     * for avoiding large object copying by mistake.
     */
    TimeServer& operator=(const TimeServer&);
    TimeServer(const TimeServer&);

};

} // namespace opencog

#endif // _OPENCOG_TIME_SERVER_H
