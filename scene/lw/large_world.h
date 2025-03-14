/**************************************************************************/
/*  large_world.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef LARGE_WORLD_H
#define LARGE_WORLD_H

#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"

class LargeWorldObject;
class LargeWorldHandler;
class Node;

//////////////////

struct LargeWorldNotification {
	enum {
		NONE,
		NEAR,
		DISTANT
	};
};

//////////////////

class LargeWorldObserver : public RefCounted {
	GDCLASS(LargeWorldObserver, RefCounted);

	friend class LargeWorldHandler;

private:
	Vector3 position;
	LargeWorldHandler *handler;
	float range;
	bool enabled;

protected:
	static void _bind_methods();

public:
	void set_position(const Vector3 &p_position);
	const Vector3 &get_position() const { return position; }
	void set_range(float p_range);
	float get_range() const { return range; }
	void set_enabled(bool p_enabled);
	bool is_enabled() const { return range > 0.0f && enabled; }

	LargeWorldHandler *get_handler() const { return handler; }

	LargeWorldObserver();
	~LargeWorldObserver();
};

//////////////////

class LargeWorldObject : public RefCounted {
	GDCLASS(LargeWorldObject, RefCounted);

	friend class LargeWorldHandler;

private:
	Vector3 position;
	LargeWorldHandler *handler;
	Node *node;
	float range;
	bool enabled : 1;
	bool distant : 1;
	bool in_near : 1;
	bool in_distant : 1;

protected:
	static void _bind_methods();
	void _large_world_notification(int p_notification);

public:
	void set_position(const Vector3 &p_position);
	const Vector3 &get_position() const { return position; }
	void set_range(float p_range);
	float get_range() const { return range; }
	void set_enabled(bool p_enabled);
	bool is_enabled() const { return range > 0.0f && enabled; }

	LargeWorldHandler *get_handler() const { return handler; }
	Node *get_node() const { return node; }
	bool is_near() const { return !distant; }

	LargeWorldObject();
	~LargeWorldObject();
};

//////////////////

class LargeWorldHandler : public RefCounted {
	GDCLASS(LargeWorldHandler, RefCounted);

	friend class LargeWorldObserver;
	friend class LargeWorldObject;

private:
	LocalVector<Ref<LargeWorldObserver>> observers;
	LocalVector<Ref<LargeWorldObject>> objects;
	LocalVector<LargeWorldObject*> objects_near; // reused in update()
	LocalVector<LargeWorldObject*> objects_distant; // reused in update()
	bool need_updates;

	void _remove_all_observers_and_objects();
	void _invalidate_observer(LargeWorldObserver *p_observer);
	void _invalidate_object(LargeWorldObject *p_object);

protected:
	static void _bind_methods();

public:
	Ref<LargeWorldObserver> create_observer(const Vector3 &p_position = Vector3(0.0f, 0.0f, 0.0f), float p_range = 0.0f);
	void remove_observer(Ref<LargeWorldObserver> observer);
	LocalVector<Ref<LargeWorldObserver>> get_observers() const { return observers; }

	Ref<LargeWorldObject> create_object(Node *node, const Vector3 &p_position = Vector3(0.0f, 0.0f, 0.0f), float p_range = 0.0f);
	void remove_object(Ref<LargeWorldObject> object);
	LocalVector<Ref<LargeWorldObject>> get_objects() const { return objects; }

	void update();

	LargeWorldHandler();
	~LargeWorldHandler();
};

#endif
