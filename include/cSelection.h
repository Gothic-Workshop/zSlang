#ifndef CSELECTION_H
#define CSELECTION_H

#include<boost\regex.hpp>
#include<string>
#include<set>

using std::string;
using std::set;
using std::vector;

class TChunkFilter;
class zWorld;
class zChunkSelectable;

class cSelection
{
    public:
        ~cSelection() {};

        /* construction */
        static cSelection all(zWorld *world);
        static cSelection wps(zWorld *world);
        static cSelection vobs(zWorld *world);
        static cSelection complementOf(zWorld *world, cSelection *other);
        static cSelection byFilter(zWorld *world, const TChunkFilter &filter);
        static cSelection byProperty(zWorld *world,
                                       const string &name,
                                       const string &value);

        /* alteration */
        cSelection& filter(const TChunkFilter &filter);
        cSelection& subtract(const cSelection &other);
        cSelection& add (const cSelection &other);
        cSelection& intersect(const cSelection &other);

        cSelection& insert(zChunkSelectable* chunk) {
            if (!chunk) return *this;
            selection.insert(chunk);
            return *this;
        }
        cSelection& remove(zChunkSelectable* chunk) {
            selection.erase(chunk);
            return *this;
        }

        public:
            static bool subsetOf(const cSelection &sel1, const cSelection &sel2);
            bool operator==(const cSelection &other) const {
                return this->selection.size() == other.selection.size()
                && subsetOf(*this, other);
            }
            bool operator!=(const cSelection &other) const {
                return !(*this == other);
            }
            bool operator<=(const cSelection &other) const {
                return subsetOf(*this,other);
            }
            bool operator>=(const cSelection &other) const {
                return subsetOf(other,*this);
            }
            bool operator<(const cSelection &other) {
                return this->selection.size() < other.selection.size()
                && subsetOf(*this, other);
            }
            bool operator>(const cSelection &other) {
                return this->selection.size() > other.selection.size()
                && subsetOf(other, *this);
            }

        unsigned int size() const { return selection.size(); };
        bool contains(zChunkSelectable * chunk) const;

        cSelection() {}; //create empty selection

        /* iterating */
        private: typedef set<zChunkSelectable*> TVobSet;
        public : typedef set<zChunkSelectable*>::const_iterator iterator;

        iterator begin() const { return selection.begin(); }
        iterator end()  const  { return selection.end();   }
    protected:
    private:
        set<zChunkSelectable*> selection;

    friend class zWorld;
};

//***************************
// Filters
//***************************

class TChunkFilter {
    public:
        virtual bool test(zChunkSelectable *chunk) const = 0;
    protected:
        TChunkFilter() {};
};

class TPropertyFilter : public TChunkFilter {
    public:
        TPropertyFilter(const string &name, const string &value)
                        : name(name), value(value) { }

        bool test(zChunkSelectable *chunk) const;
    private:
        string name, value;
};

class TSelectionFilter : public TChunkFilter {
    public:
        TSelectionFilter(const cSelection *sel) : sel(sel) { }
        bool test(zChunkSelectable *chunk) const {
            return sel->contains(chunk);
        }
    private:
        const cSelection *sel;
};

class TSelectionComplementFilter : public TChunkFilter {
    public:
        TSelectionComplementFilter(const cSelection *sel) : sel(sel) { }

        bool test(zChunkSelectable *chunk) const {
            return !sel->contains(chunk);
        }
    private:
        const cSelection *sel;
};

class TRegExFilter : public TChunkFilter {
    public:
        TRegExFilter(const string &property, const string &regEx)
                    : property(property), regEx(regEx) { }

        bool test(zChunkSelectable *chunk) const;
    private:
        string property;
        boost::regex regEx;
};

class TClassFilter : public TChunkFilter {
    public:
        TClassFilter(const string &regEx) : regEx(regEx) {}
        bool test(zChunkSelectable *chunk) const;
    private:
        boost::regex regEx;
};

template<class T>
class TPredicateFilter : public TChunkFilter {
    public:
        TPredicateFilter(const T & pred) : pred(pred) { }
        bool test(zChunkSelectable * chunk) const {
            return pred(chunk);
        }
    private:
        T pred;
};

#endif // CSELECTION_H
