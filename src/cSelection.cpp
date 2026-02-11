#include "cSelection.h"
#include<boost\regex.hpp>
#include<zWorld.h>
#include<cProperty.h>
#include<zChunkSelectable.h>
#include<cLogger.h>

typedef set<zChunkSelectable*>::iterator TSelIt;

cSelection cSelection::byFilter(zWorld *world, const TChunkFilter &filter) {
    cSelection sel;

    for (zWorld::iterator it = world->begin(); it != world->end(); ++it) {
        if (filter.test(*it)) {
            sel.selection.insert(*it);
        }
    }

    return sel;
}

cSelection cSelection::byProperty(zWorld *world,
                                    const string &name,
                                    const string &value) {
    cSelection sel;
    if (cProperty::getChunks(name, value, world, sel.selection)) {
        return sel;
    } else {
        /* damn it, the property is not hashed.
         * do it differently, expensive approach: */
        TPropertyFilter filter(name, value);
        return byFilter(world, filter);
    }
}

cSelection cSelection::all(zWorld *world) {
    cSelection sel;

    if (!world) {
        return sel;
    }

    for (zWorld::iterator it = world->begin(); it != world->end(); ++it) {
        sel.selection.insert(*it);
    }

    return sel;
}

cSelection cSelection::vobs(zWorld *world) {
    cSelection sel;

    for (zWorld::iterator it = world->begin(); it != world->beginWaynet(); ++it) {
        sel.selection.insert(*it);
    }

    return sel;
}

cSelection cSelection::wps(zWorld *world) {
    cSelection sel;

    for (zWorld::iterator it = world->beginWaynet(); it != world->end(); ++it) {
        sel.selection.insert(*it);
    }

    return sel;
}

cSelection cSelection::complementOf(zWorld *world, cSelection *sel) {
    TSelectionComplementFilter filter(sel);

    return byFilter(world, filter);
}

cSelection& cSelection::filter(const TChunkFilter &filter) {
    TSelIt it = this->selection.begin();
    TSelIt deleteIt;

    while (it != this->selection.end()) {
        if (!filter.test(*it)) {
            /* iterator judgling, iterator given to erase is invalidated
             * other remain valid(!) */
            deleteIt = it++;
            this->selection.erase(deleteIt);
        } else {
            it++;
        }
    }

    return *this;
}

cSelection& cSelection::subtract(const cSelection &other) {
    /* let n = size(other), m size(this) */

    if (/* n */ other.size() > /* m */ this->size()) {
        /* this is small, filter them; m log n */
        TSelectionComplementFilter filter(&other);
        this->filter(filter);
    } else {
        /* other is small, search and delete selectively; n log m */
        for (TSelIt it = other.selection.begin();
             it != other.selection.end(); ++it) {
            this->selection.erase(*it);
        }
    }
    return *this;
}

cSelection& cSelection::add(const cSelection &other) {
    this->selection.insert(other.selection.begin(), other.selection.end());
    return *this;
}

cSelection& cSelection::intersect(const cSelection &other) {
    #define isMuchSmallerThan * 2 + 100 <
    if (/* n */ other.size() isMuchSmallerThan /* m */ this->size()) {
        /* this approach involves a lot of copying */
        cSelection thisCopy = *this; /* copy! */
        this->selection = other.selection; /* copy! */

        TSelectionFilter filter(&thisCopy);
        this->filter(filter);
        /* n log m + copying Time */
    } else {
        TSelectionFilter filter(&other);
        this->filter(filter);
    }
    return *this;
    #undef isMuchSmallerThan
}

bool cSelection::contains(zChunkSelectable * chunk) const {
    return this->selection.count(chunk);
}

//***************************
// Die verschiedenen Filter:
//***************************

bool TPropertyFilter::test(zChunkSelectable *chunk) const {
     cProperty *prop = chunk->getProperty(name);
     return prop && prop->toString() == value;
}

bool TRegExFilter::test(zChunkSelectable *chunk) const {
    cProperty * prop = chunk->getProperty(property);
    return prop && boost::regex_search(prop->toString(), regEx);
}

bool TClassFilter::test(zChunkSelectable *chunk) const {
    return boost::regex_search(chunk->getClassName(), regEx);
}

//***************************
// Subset
//***************************

bool cSelection::subsetOf(const cSelection &sel1, const cSelection &sel2) {
    /* trivial case: */
    if (sel1.size() > sel2.size()) {
        return false;
    }

    TSelIt it1 = sel1.selection.begin();
    TSelIt it2 = sel2.selection.begin();

    TSelIt it1End = sel1.selection.end();
    TSelIt it2End = sel2.selection.end();

    while (it1 != it1End && it2 != it2End) {
        if (*it1 < *it2) {
            //element in sel1 is not in sel2
            return false;
        } else if (*it1 > *it2) {
            //element in sel2 is not in sel1
            ++it2;
        } else {
            //element in sel1 and in sel2
            ++it1;
            ++it2;
        }
    }

    //found all elements of it1 in it2.
    return it1 == it1End;
}
