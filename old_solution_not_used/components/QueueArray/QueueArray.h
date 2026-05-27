/*
 *  QueueArray.h
 *	Inspired by some Arduino messy original
 *
 *	https://www.cplusplus.com/doc/oldtutorial/templates/
 *	Templates and multiple-file projects
From the point of view of the compiler, templates are not normal functions or classes. They are compiled on demand, meaning that the code of a template function is not compiled until an instantiation with specific template arguments is required. At that moment, when an instantiation is required, the compiler generates a function specifically for those arguments from the template.
When projects grow it is usual to split the code of a program in different source code files. In these cases, the interface and implementation are generally separated. Taking a library of functions as example, the interface generally consists of declarations of the prototypes of all the functions that can be called. These are generally declared in a "header file" with a .h extension, and the implementation (the definition of these functions) is in an independent file with c++ code.
Because templates are compiled when required, this forces a restriction for multi-file projects: the implementation (definition) of a template class or function must be in the same file as its declaration. That means that we cannot separate the interface in a separate header file, and that we must include both interface and implementation in any file that uses the templates.
Since no code is generated until a template is instantiated when required, compilers are prepared to allow the inclusion more than once of the same template file with both declarations and definitions in a project without generating linkage errors.
 */

// header defining the interface of the source.
#ifndef _QUEUEARRAY__H
#define _QUEUEARRAY__H

#define QUEUEARRAY_INIT_SIZE 4

template<typename T>
class QueueArray {
  public:
    // init the queue (constructor).
    QueueArray (int aInitialSize = QUEUEARRAY_INIT_SIZE, bool aAutoShrink = false);
    // clear the queue (destructor).
    ~QueueArray ();
    // add an item to the queue.
    void enqueue (const T i);
    // remove an item from the queue.
    T dequeue ();
    // push an item to the queue.
    void push (const T i);
    // pop an item from the queue.
    T pop ();
    // get the front of the queue.
    T front () const;
    // get an item from the queue.
    T peek () const;
    // check if the queue is empty.
    bool isEmpty () const;
    // get the number of items in the queue.
    int count () const;
    // check if the queue is full.
    bool isFull () const;

    void shrink();

  private:
    // resize the size of the queue.
    void resize (const int s);
//    // the initial size of the queue.
//    static const int initialSize = 3;
    T * contents;    // the array of the queue.
    int size;        // the size of the queue.
    int items;       // the number of items of the queue.
    int head;        // the head of the queue.
    int tail;        // the tail of the queue.
    bool autoshrink;	//shink size automaticly after dequeue if lower than
};

#include "esp_log.h"

// init the queue (constructor).
template<typename T>
QueueArray<T>::QueueArray (int aInitialSize, bool aAutoShrink) {
  size = 0;       // set the size of queue to zero.
  items = 0;      // set the number of items of queue to zero.
  head = 0;       // set the head of the queue to zero.
  tail = 0;       // set the tail of the queue to zero.
  // allocate enough memory for the array.
  contents = (T *) malloc (sizeof (T) * aInitialSize);
  // if there is a memory allocation error.
  if (contents == NULL){
    ESP_LOGE("QueueArray", "malloc failed");
    return;
  }
  // set the initial size of the queue.
  size = aInitialSize;
  autoshrink = aAutoShrink;
}

// clear the queue (destructor).
template<typename T>
QueueArray<T>::~QueueArray () {
  free (contents); // deallocate the array of the queue.
  contents = NULL; // set queue's array pointer to nowhere.
  size = 0;        // set the size of queue to zero.
  items = 0;       // set the number of items of queue to zero.
  head = 0;        // set the head of the queue to zero.
  tail = 0;        // set the tail of the queue to zero.
}

// resize the size of the queue.
template<typename T>
void QueueArray<T>::resize (const int s) {
  if (s <= 0){
	ESP_LOGE("QueueArray", "invalid size");
	return;
  }
  // allocate enough memory for the temporary array.
  T * temp = (T *) malloc (sizeof (T) * s);

  // if there is a memory allocation error.
  if (temp == NULL){
	ESP_LOGE("QueueArray", "insufficient memory to initialize temporary queue");
	return;
  }
  // copy the items from the old queue to the new one.
  for (int i = 0; i < items; i++)
    temp[i] = contents[(head + i) % size];
  // deallocate the old array of the queue.
  free (contents);
  // copy the pointer of the new queue.
  contents = temp;
  // set the head and tail of the new queue.
  head = 0; tail = items;
  // set the new size of the queue.
  size = s;
}

// add an item to the queue.
template<typename T>
void QueueArray<T>::enqueue (const T i) {
  // check if the queue is full.
  if (isFull ())
    // double size of array.
    resize (size * 2);

  // store the item to the array.
  contents[tail++] = i;

  // wrap-around index.
  if (tail == size) tail = 0;

  // increase the items.
  items++;
}

// push an item to the queue.
template<typename T>
void QueueArray<T>::push (const T i) {
  enqueue(i);
}

// remove an item from the queue.
template<typename T>
T QueueArray<T>::dequeue () {
  // check if the queue is empty.
  if (isEmpty())
	  return T();
  // fetch the item from the array.
  T item = contents[head++];
  // decrease the items.
  items--;
  // wrap-around index.
  if (head == size) head = 0;
  // shrink size of array if necessary.
  if (autoshrink && !isEmpty() && (items <= size / 4))
    resize (size / 2);
  // return the item from the array.
  return item;
}

template<typename T>
void QueueArray<T>::shrink(){
	if (!isEmpty())
	  resize(items);
}

// pop an item from the queue.
template<typename T>
T QueueArray<T>::pop () {
  return dequeue();
}

// get the front of the queue.
template<typename T>
T QueueArray<T>::front () const {
  // check if the queue is empty.
  if (isEmpty ())
	  return NULL;
  // get the item from the array.
  return contents[head];
}

// get an item from the queue.
template<typename T>
T QueueArray<T>::peek () const {
  return front();
}

// check if the queue is empty.
template<typename T>
bool QueueArray<T>::isEmpty () const {
  return items == 0;
}

// check if the queue is full.
template<typename T>
bool QueueArray<T>::isFull () const {
  return items == size;
}

// get the number of items in the queue.
template<typename T>
int QueueArray<T>::count () const {
  return items;
}


#endif // _QUEUEARRAY__H
