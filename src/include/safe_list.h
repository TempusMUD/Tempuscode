#ifndef _SAFE_LIST_H_
#define _SAFE_LIST_H_

#include <stdlib.h>
#include <unistd.h>
#include <list>
#include <algorithm>

using namespace std;

/**
 *  A Templated, doubly-linked, type-safe list that provides "safe iterators" 
 *  into it's nodes.  A "safe iterator" is defined as one that is incremented
 *  any time the node it "points to" is removed.
**/
template <class T> class SafeList:protected list <T> {
  public:
	/** 
     *  A 'safe iterator' for use with this safelist
     *
    **/
  class iterator:public list <T>::iterator {
	  private:					// Data
		SafeList <T> *_list;
		bool _saved;			// iterator has been "saved" from pointing
		// at an invalid node
		friend class SafeList <T>;
	  public:
		/**
		 *  Creates a fresh new iterator
		**/
	    iterator():list <T>::iterator() {
			_saved = false;
			_list = NULL;
		}
		/**
		 *  Creates a fresh new iterator just like the one you have.
		 *  it - The iterator to copy.
		**/
		iterator(const iterator & it) {
			_saved = false;
			_list = NULL;
			*this = it;
		}
		/**
		 *  Creates a fresh new iterator pointing to the beginning of
		 *  this list.
		 *  l - The list to register this iterator with.
		**/
	    iterator(SafeList <T> *l):list <T>::iterator() {
			_saved = false;
			list <T>::iterator::operator = (((list <T> *)l)->begin());
			_list = l;
			if (_list != NULL)
				_list->addIterator(this);
		}
		/**
		 *  Destroys the iterator, unregistering it from it's list if needed.
		**/
		~iterator() {
			if (_list != NULL)
				_list->removeIterator(this);
			_list = NULL;
		}
		/** 
		 *  Postincrements this iterator.
		 *  If iterator was previously "saved" then it is set unsaved rather
		 *    than incremented again.
		 *  Segfaults if no list to iterate through
		**/
		iterator operator++ (int) {	// post
			if (_saved) {
				_saved = false;
				return *this;
			}
			iterator it(*this);
			++(*this);
			return it;
		}
		/** 
		 *  Preincrements this iterator.
		 *  If iterator was previously "saved" then it is set unsaved rather
		 *    than incremented again.
		 *  Segfaults if no list to iterate through
		**/
		iterator & operator++ () {	// preincrement
			// this iterator has been saved and shouldn't move yet.
			if (_saved) {
				_saved = false;
				return *this;
			}
			// Increment the superclass
			list <T>::iterator::operator++ ();
			return *this;
		}
		/**  Assignment **/ 
		iterator & operator = (const iterator & it) {
			// superclass assignment
			list <T>::iterator::operator = (it);
			// If pointing to a list, unregister
			if (_list != NULL)
				_list->removeIterator(this);
			_list = it._list;
			// If pointing to a list, register
			if (_list != NULL)
				_list->addIterator(this);
			_saved = false;
			return *this;
		}
	  private:					// Functions
		void operator = (const typename list <T>::iterator & it){
			// If pointing to a list, unregister
			if (_list != NULL)
				_list->removeIterator(this);
			_list = NULL;
			list <T>::iterator::operator = (it);
		}
		void save() {
			_saved = false;
			++(*this);
			_saved = true;
		}
	};							// End SafeList::iterator
  public:
  SafeList(bool prepend = false):list <T> (), _iterators() {
		_end = list <T>::end();
		_end._list = this;
		_prepend = prepend;
	}
	// Upgrades from list<T>
	list <T>::size;
	list <T>::insert;
	// list<T>'s begin and end dont work quite right
	iterator begin() {
		return iterator(this);
	}
	iterator & end() {
		return _end;
	}
	/**
     * Adds a node to the SafeList either by prepending or appending.
    **/
	void add(T c) {
		if (_prepend)
			push_front(c);
		else
			push_back(c);
	}

	/**
	 *  Overridden to avoid an odd bug in list<T>::empty()
	**/
	bool empty() {
		return (size() == 0);
	}

	void remove(T c) {
		iterator it = find(begin(), end(), c);
		if (it != _end) {
			//Save iterators from being orphaned by a node removal.
			removeUpdate(it);
			list <T>::erase(it);
		}
	}
	void remove(iterator & it) {
		iterator tempi(it);		// Store the position to remove
		//Save iterators from being orphaned by a node removal.
		removeUpdate(tempi, 1);
		//it.save();// Move iterator out of the way
		list <T>::erase(tempi);
	}
	void addIterator(iterator * it) {
		_iterators.push_front(it);
	}
	void removeIterator(iterator * it) {
		_iterators.remove(it);
	}

  protected:
	// If true, push_front to add a node.
	// If false, push_back to add a node
	bool _prepend;
  private:
	// "saves" all iterators pointing at ths given position <it>
	// temps - the number of temp iterators used before calling removeUpdate
	void removeUpdate(iterator & it, unsigned int temps = 0) {
		// No need to notify if there's only one iterator.
		if (_iterators.size() <= (1 + temps)) {
			return;
		}

		typename list <iterator *>::iterator sit = _iterators.begin();
		for (; sit != _iterators.end(); ++sit) {
			if (*sit != &it && *(*sit) == it) {
				(*sit)->save();
			}
		}
	}
	list <iterator *>_iterators;
	iterator _end;
};

#endif
