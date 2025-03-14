/**************************************************************************/
/*  large_world.cpp                                                       */
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

#include "large_world.h"
#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "scene/main/node.h"

//////////////////

void LargeWorldObserver::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_position", "new_position"), &LargeWorldObserver::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &LargeWorldObserver::get_position);
	ClassDB::bind_method(D_METHOD("set_range", "new_range"), &LargeWorldObserver::set_range);
	ClassDB::bind_method(D_METHOD("get_range"), &LargeWorldObserver::get_range);
	ClassDB::bind_method(D_METHOD("set_enabled", "new_range"), &LargeWorldObserver::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &LargeWorldObserver::is_enabled);
	ClassDB::bind_method(D_METHOD("get_handler"), &LargeWorldObserver::get_handler);
}

void LargeWorldObserver::set_position(const Vector3 &p_position) {
	ERR_FAIL_COND_MSG(handler == nullptr, "Tried to update the position of a large world observer without a large world handler attached");
	if (p_position.distance_squared_to(position) <= (real_t)CMP_EPSILON) {
		return;
	}
	position = p_position;
	handler->_invalidate_observer(this);
}

void LargeWorldObserver::set_range(float p_range) {
	ERR_FAIL_COND_MSG(handler == nullptr, "Tried to update the position of a large world observer without a large world handler attached");
	if (p_range < 0.0f) {
		p_range = 0.0f;
	}
	if (Math::abs(p_range - range) <= (real_t)CMP_EPSILON) {
		return;
	}
	range = p_range;
	handler->_invalidate_observer(this);
}

void LargeWorldObserver::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

LargeWorldObserver::LargeWorldObserver() :
		position(0.0f, 0.0f, 0.0f), handler(nullptr), range(0.0f), enabled(true) {
}

LargeWorldObserver::~LargeWorldObserver() {
	ERR_FAIL_COND_MSG(handler != nullptr, "Large world observer destroyed while attached to a large world handler");
}

//////////////////

void LargeWorldObject::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_position", "new_position"), &LargeWorldObject::set_position);
	ClassDB::bind_method(D_METHOD("get_position"), &LargeWorldObject::get_position);
	ClassDB::bind_method(D_METHOD("set_range", "new_range"), &LargeWorldObject::set_range);
	ClassDB::bind_method(D_METHOD("get_range"), &LargeWorldObject::get_range);
	ClassDB::bind_method(D_METHOD("set_enabled", "new_range"), &LargeWorldObject::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &LargeWorldObject::is_enabled);
	ClassDB::bind_method(D_METHOD("is_near"), &LargeWorldObject::is_near);
	ClassDB::bind_method(D_METHOD("get_handler"), &LargeWorldObject::get_handler);
}

void LargeWorldObject::_large_world_notification(int p_notification) {
	node->_large_world_notification(p_notification);
}

void LargeWorldObject::set_position(const Vector3 &p_position) {
	ERR_FAIL_COND_MSG(handler == nullptr, "Tried to update the position of a large world object without a large world handler attached");
	if (p_position.distance_squared_to(position) <= (real_t)CMP_EPSILON) {
		return;
	}
	position = p_position;
	handler->_invalidate_object(this);
}

void LargeWorldObject::set_range(float p_range) {
	ERR_FAIL_COND_MSG(handler == nullptr, "Tried to update the position of a large world object without a large world handler attached");
	if (p_range < 0.0f) {
		p_range = 0.0f;
	}
	if (Math::abs(p_range - range) <= (real_t)CMP_EPSILON) {
		return;
	}
	range = p_range;
	handler->_invalidate_object(this);
}

void LargeWorldObject::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

LargeWorldObject::LargeWorldObject() :
		position(0.0f, 0.0f, 0.0f), handler(nullptr), node(nullptr), range(0.0f), enabled(true), distant(true), in_near(false), in_distant(false) {
}

LargeWorldObject::~LargeWorldObject() {
	ERR_FAIL_COND_MSG(handler != nullptr, "Large world object destroyed while attached to a large world handler");
	ERR_FAIL_COND_MSG(!distant, "Large world object destroyed while in near state");
}

//////////////////

void LargeWorldHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_observer", "position", "range"), &LargeWorldHandler::create_observer);
	ClassDB::bind_method(D_METHOD("remove_observer", "observer"), &LargeWorldHandler::remove_observer);
	ClassDB::bind_method(D_METHOD("update"), &LargeWorldHandler::update);
}

void LargeWorldHandler::_remove_all_observers_and_objects() {
	for (Ref<LargeWorldObject> &object : objects) {
		_invalidate_object(object.ptr());
		if (!object->distant) {
			object->distant = false;
			object->_large_world_notification(LargeWorldNotification::DISTANT);
		}
		object->handler = nullptr;
	}
	objects.clear();
	for (Ref<LargeWorldObserver> &observer : observers) {
		_invalidate_observer(observer.ptr());
		observer->handler = nullptr;
	}
	observers.clear();
}

void LargeWorldHandler::_invalidate_observer(LargeWorldObserver *p_observer) {
	need_updates = true;
}

void LargeWorldHandler::_invalidate_object(LargeWorldObject *p_object) {
	need_updates = true;
}

Ref<LargeWorldObserver> LargeWorldHandler::create_observer(const Vector3 &position, float range) {
	Ref<LargeWorldObserver> observer;
	observer.instantiate();
	observer->position = position;
	observer->handler = this;
	observer->range = range;
	observers.push_back(observer);
	need_updates = true;
	return observer;
}

void LargeWorldHandler::remove_observer(Ref<LargeWorldObserver> observer) {
	if (observer.is_null()) {
		WARN_PRINT_ONCE("Tried to remove a null large world observer");
		return;
	}
	observer->handler = nullptr;
	observers.erase(observer);
	need_updates = true;
}

Ref<LargeWorldObject> LargeWorldHandler::create_object(Node *node, const Vector3 &position, float range) {
	Ref<LargeWorldObject> object;
	object.instantiate();
	object->position = position;
	object->handler = this;
	object->node = node;
	object->range = range;
	objects.push_back(object);
	need_updates = true;
	object->_large_world_notification(LargeWorldNotification::DISTANT);
	return object;
}

void LargeWorldHandler::remove_object(Ref<LargeWorldObject> object) {
	if (object.is_null()) {
		WARN_PRINT_ONCE("Tried to remove a null large world object");
		return;
	}
	if (!object->distant) {
		object->distant = true;
		object->_large_world_notification(LargeWorldNotification::DISTANT);
	}
	object->handler = nullptr;
	object->node = nullptr;
	objects.erase(object);
	need_updates = true;
}

void LargeWorldHandler::update() {
	if (!need_updates) {
		return;
	}
	need_updates = false;
	objects_near.clear();
	objects_distant.clear();
	for (Ref<LargeWorldObject> &object : objects) {
		object->in_near = object->in_distant = false;
	}
	for (Ref<LargeWorldObserver> &observer : observers) {
		for (Ref<LargeWorldObject> &object : objects) {
			float near_distance = observer->range + object->range;
			float far_distance = near_distance + 100;
			float distance_to_position = observer->position.distance_to(object->position);
			bool go_distant = distance_to_position >= far_distance;
			bool go_near = distance_to_position <= near_distance;
			if (go_distant) {
				if (!object->distant && !object->in_near && !object->in_distant) {
					objects_distant.push_back(object.ptr());
					object->in_distant = true;
				}
			}
			if (go_near) {
				if (object->distant && !object->in_near) {
					objects_near.push_back(object.ptr());
				}
				object->in_near = true;;
			}
		}
	}
	int got_in = 0, got_out = 0;
	for (LargeWorldObject *object : objects_near) {
		object->in_near = false;
		object->in_distant = false;
		object->distant = false;
		object->_large_world_notification(LargeWorldNotification::NEAR);
		printf("marked %p as near\n", object);
		got_in++;
	}
	for (LargeWorldObject *object : objects_distant) {
		if (object->in_distant && !object->in_near) {
			object->in_distant = false;
			object->distant = true;
			object->_large_world_notification(LargeWorldNotification::DISTANT);
			printf("marked %p as distant\n", object);
			got_out++;
		}
	}
	if (got_in || got_out) {
		printf("large world update: in=%i out=%i total=%i observers=%i\n", got_in, got_out, objects.size(), observers.size());
	}
}

LargeWorldHandler::LargeWorldHandler() :
		need_updates(true) {
}

LargeWorldHandler::~LargeWorldHandler() {
	_remove_all_observers_and_objects();
}
