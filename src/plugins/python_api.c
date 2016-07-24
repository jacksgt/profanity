/*
 * python_api.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include "config.h"

#include <Python.h>
#include <frameobject.h>

#include <glib.h>

#include "plugins/api.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "log.h"

static char* _python_plugin_name(void);

static PyObject*
python_api_cons_alert(PyObject *self, PyObject *args)
{
    allow_python_threads();
    api_cons_alert();
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_cons_show(PyObject *self, PyObject *args)
{
    PyObject* message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_cons_show(message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_cons_show_themed(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "OOOO", &group, &key, &def, &message)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_cons_show_themed(group_str, key_str, def_str, message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_cons_bad_cmd_usage(PyObject *self, PyObject *args)
{
    PyObject *cmd = NULL;
    if (!PyArg_ParseTuple(args, "O", &cmd)) {
        return Py_BuildValue("");
    }

    char *cmd_str = python_str_or_unicode_to_string(cmd);

    allow_python_threads();
    api_cons_bad_cmd_usage(cmd_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_register_command(PyObject *self, PyObject *args)
{
    PyObject *command_name = NULL;
    int min_args = 0;
    int max_args = 0;
    PyObject *synopsis = NULL;
    PyObject *description = NULL;
    PyObject *arguments = NULL;
    PyObject *examples = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "OiiOOOOO", &command_name, &min_args, &max_args,
            &synopsis, &description, &arguments, &examples, &p_callback)) {
        return Py_BuildValue("");
    }

    char *command_name_str = python_str_or_unicode_to_string(command_name);
    char *description_str = python_str_or_unicode_to_string(description);

    char *plugin_name = _python_plugin_name();
    log_debug("Register command %s for %s", command_name_str, plugin_name);

    if (p_callback && PyCallable_Check(p_callback)) {
        Py_ssize_t len = PyList_Size(synopsis);
        const char *c_synopsis[len == 0 ? 0 : len+1];
        Py_ssize_t i = 0;
        for (i = 0; i < len; i++) {
            PyObject *item = PyList_GetItem(synopsis, i);
            char *c_item = python_str_or_unicode_to_string(item);
            c_synopsis[i] = c_item;
        }
        c_synopsis[len] = NULL;

        Py_ssize_t args_len = PyList_Size(arguments);
        const char *c_arguments[args_len == 0 ? 0 : args_len+1][2];
        i = 0;
        for (i = 0; i < args_len; i++) {
            PyObject *item = PyList_GetItem(arguments, i);
            Py_ssize_t len2 = PyList_Size(item);
            if (len2 != 2) {
                return Py_BuildValue("");
            }
            PyObject *arg = PyList_GetItem(item, 0);
            char *c_arg = python_str_or_unicode_to_string(arg);
            c_arguments[i][0] = c_arg;

            PyObject *desc = PyList_GetItem(item, 1);
            char *c_desc = python_str_or_unicode_to_string(desc);
            c_arguments[i][1] = c_desc;
        }

        c_arguments[args_len][0] = NULL;
        c_arguments[args_len][1] = NULL;

        len = PyList_Size(examples);
        const char *c_examples[len == 0 ? 0 : len+1];
        i = 0;
        for (i = 0; i < len; i++) {
            PyObject *item = PyList_GetItem(examples, i);
            char *c_item = python_str_or_unicode_to_string(item);
            c_examples[i] = c_item;
        }
        c_examples[len] = NULL;

        allow_python_threads();
        api_register_command(plugin_name, command_name_str, min_args, max_args, c_synopsis,
            description_str, c_arguments, c_examples, p_callback, python_command_callback, NULL);
        disable_python_threads();
    }

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject *
python_api_register_timed(PyObject *self, PyObject *args)
{
    PyObject *p_callback = NULL;
    int interval_seconds = 0;

    if (!PyArg_ParseTuple(args, "Oi", &p_callback, &interval_seconds)) {
        return Py_BuildValue("");
    }

    char *plugin_name = _python_plugin_name();
    log_debug("Register timed for %s", plugin_name);

    if (p_callback && PyCallable_Check(p_callback)) {
        allow_python_threads();
        api_register_timed(plugin_name, p_callback, interval_seconds, python_timed_callback, NULL);
        disable_python_threads();
    }

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject *
python_api_completer_add(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;
    PyObject *items = NULL;

    if (!PyArg_ParseTuple(args, "OO", &key, &items)) {
        return Py_BuildValue("");
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete add %s for %s", key_str, plugin_name);

    Py_ssize_t len = PyList_Size(items);
    char *c_items[len];

    Py_ssize_t i = 0;
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GetItem(items, i);
        char *c_item = python_str_or_unicode_to_string(item);
        c_items[i] = c_item;
    }
    c_items[len] = NULL;

    allow_python_threads();
    api_completer_add(plugin_name, key_str, c_items);
    disable_python_threads();

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject *
python_api_completer_remove(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;
    PyObject *items = NULL;

    if (!PyArg_ParseTuple(args, "OO", &key, &items)) {
        return Py_BuildValue("");
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete remove %s for %s", key_str, plugin_name);

    Py_ssize_t len = PyList_Size(items);
    char *c_items[len];

    Py_ssize_t i = 0;
    for (i = 0; i < len; i++) {
        PyObject *item = PyList_GetItem(items, i);
        char *c_item = python_str_or_unicode_to_string(item);
        c_items[i] = c_item;
    }
    c_items[len] = NULL;

    allow_python_threads();
    api_completer_remove(plugin_name, key_str, c_items);
    disable_python_threads();

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject *
python_api_completer_clear(PyObject *self, PyObject *args)
{
    PyObject *key = NULL;

    if (!PyArg_ParseTuple(args, "O", &key)) {
        return Py_BuildValue("");
    }

    char *key_str = python_str_or_unicode_to_string(key);

    char *plugin_name = _python_plugin_name();
    log_debug("Autocomplete clear %s for %s", key_str, plugin_name);

    allow_python_threads();
    api_completer_clear(plugin_name, key_str);
    disable_python_threads();

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject*
python_api_notify(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    PyObject *category = NULL;
    int timeout_ms = 5000;

    if (!PyArg_ParseTuple(args, "OiO", &message, &timeout_ms, &category)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);
    char *category_str = python_str_or_unicode_to_string(category);

    allow_python_threads();
    api_notify(message_str, category_str, timeout_ms);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_send_line(PyObject *self, PyObject *args)
{
    PyObject *line = NULL;
    if (!PyArg_ParseTuple(args, "O", &line)) {
        return Py_BuildValue("");
    }

    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_send_line(line_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_get_current_recipient(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *recipient = api_get_current_recipient();
    disable_python_threads();
    if (recipient) {
        return Py_BuildValue("s", recipient);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject *
python_api_get_current_muc(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *room = api_get_current_muc();
    disable_python_threads();
    if (room) {
        return Py_BuildValue("s", room);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject *
python_api_get_current_nick(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char *nick = api_get_current_nick();
    disable_python_threads();
    if (nick) {
        return Py_BuildValue("s", nick);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject*
python_api_get_current_occupants(PyObject *self, PyObject *args)
{
    allow_python_threads();
    char **occupants = api_get_current_occupants();
    disable_python_threads();
    PyObject *result = PyList_New(0);
    if (occupants) {
        int len = g_strv_length(occupants);
        int i = 0;
        for (i = 0; i < len; i++) {
            PyList_Append(result, Py_BuildValue("s", occupants[i]));
        }
        return result;
    } else {
        return result;
    }
}

static PyObject*
python_api_current_win_is_console(PyObject *self, PyObject *args)
{
    allow_python_threads();
    int res = api_current_win_is_console();
    disable_python_threads();
    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject *
python_api_log_debug(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_debug(message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_log_info(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_info(message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_log_warning(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_warning(message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_log_error(PyObject *self, PyObject *args)
{
    PyObject *message = NULL;
    if (!PyArg_ParseTuple(args, "O", &message)) {
        return Py_BuildValue("");
    }

    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_log_error(message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_win_exists(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    if (!PyArg_ParseTuple(args, "O", &tag)) {
        return Py_BuildValue("");
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    allow_python_threads();
    gboolean exists = api_win_exists(tag_str);
    disable_python_threads();

    if (exists) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject *
python_api_win_create(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *p_callback = NULL;

    if (!PyArg_ParseTuple(args, "OO", &tag, &p_callback)) {
        return Py_BuildValue("");
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    char *plugin_name = _python_plugin_name();
    log_debug("Win create %s for %s", tag_str, plugin_name);

    if (p_callback && PyCallable_Check(p_callback)) {
        allow_python_threads();
        api_win_create(plugin_name, tag_str, p_callback, python_window_callback, NULL);
        disable_python_threads();
    }

    free(plugin_name);

    return Py_BuildValue("");
}

static PyObject *
python_api_win_focus(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;

    if (!PyArg_ParseTuple(args, "O", &tag)) {
        return Py_BuildValue("");
    }

    char *tag_str = python_str_or_unicode_to_string(tag);

    allow_python_threads();
    api_win_focus(tag_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_win_show(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *line = NULL;

    if (!PyArg_ParseTuple(args, "OO", &tag, &line)) {
        return Py_BuildValue("");
    }

    char *tag_str = python_str_or_unicode_to_string(tag);
    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_win_show(tag_str, line_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject *
python_api_win_show_themed(PyObject *self, PyObject *args)
{
    PyObject *tag = NULL;
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;
    PyObject *line = NULL;

    if (!PyArg_ParseTuple(args, "OOOOO", &tag, &group, &key, &def, &line)) {
        python_check_error();
        return Py_BuildValue("");
    }

    char *tag_str = python_str_or_unicode_to_string(tag);
    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);
    char *line_str = python_str_or_unicode_to_string(line);

    allow_python_threads();
    api_win_show_themed(tag_str, group_str, key_str, def_str, line_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_send_stanza(PyObject *self, PyObject *args)
{
    PyObject *stanza = NULL;
    if (!PyArg_ParseTuple(args, "O", &stanza)) {
        return Py_BuildValue("O", Py_False);
    }

    char *stanza_str = python_str_or_unicode_to_string(stanza);

    allow_python_threads();
    int res = api_send_stanza(stanza_str);
    disable_python_threads();
    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_settings_get_boolean(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *defobj = NULL;

    if (!PyArg_ParseTuple(args, "OOO!", &group, &key, &PyBool_Type, &defobj)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    int def = PyObject_IsTrue(defobj);

    allow_python_threads();
    int res = api_settings_get_boolean(group_str, key_str, def);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("O", Py_True);
    } else {
        return Py_BuildValue("O", Py_False);
    }
}

static PyObject*
python_api_settings_set_boolean(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *valobj = NULL;

    if (!PyArg_ParseTuple(args, "OOO!", &group, &key, &PyBool_Type, &valobj)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    int val = PyObject_IsTrue(valobj);

    allow_python_threads();
    api_settings_set_boolean(group_str, key_str, val);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_settings_get_string(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *def = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &def)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *def_str = python_str_or_unicode_to_string(def);

    allow_python_threads();
    char *res = api_settings_get_string(group_str, key_str, def_str);
    disable_python_threads();

    if (res) {
        return Py_BuildValue("s", res);
    } else {
        return Py_BuildValue("");
    }
}

static PyObject*
python_api_settings_set_string(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    PyObject *val = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &group, &key, &val)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);
    char *val_str = python_str_or_unicode_to_string(val);

    allow_python_threads();
    api_settings_set_string(group_str, key_str, val_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_settings_get_int(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    int def = 0;

    if (!PyArg_ParseTuple(args, "OOi", &group, &key, &def)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    int res = api_settings_get_int(group_str, key_str, def);
    disable_python_threads();

    return Py_BuildValue("i", res);
}

static PyObject*
python_api_settings_set_int(PyObject *self, PyObject *args)
{
    PyObject *group = NULL;
    PyObject *key = NULL;
    int val = 0;

    if (!PyArg_ParseTuple(args, "OOi", &group, &key, &val)) {
        return Py_BuildValue("");
    }

    char *group_str = python_str_or_unicode_to_string(group);
    char *key_str = python_str_or_unicode_to_string(key);

    allow_python_threads();
    api_settings_set_int(group_str, key_str, val);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_incoming_message(PyObject *self, PyObject *args)
{
    PyObject *barejid = NULL;
    PyObject *resource = NULL;
    PyObject *message = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &barejid, &resource, &message)) {
        return Py_BuildValue("");
    }

    char *barejid_str = python_str_or_unicode_to_string(barejid);
    char *resource_str = python_str_or_unicode_to_string(resource);
    char *message_str = python_str_or_unicode_to_string(message);

    allow_python_threads();
    api_incoming_message(barejid_str, resource_str, message_str);
    disable_python_threads();

    return Py_BuildValue("");
}

static PyObject*
python_api_disco_add_feature(PyObject *self, PyObject *args)
{
    PyObject *feature = NULL;
    if (!PyArg_ParseTuple(args, "O", &feature)) {
        return Py_BuildValue("");
    }

    char *feature_str = python_str_or_unicode_to_string(feature);

    allow_python_threads();
    api_disco_add_feature(feature_str);
    disable_python_threads();

    return Py_BuildValue("");
}

void
python_command_callback(PluginCommand *command, gchar **args)
{
    disable_python_threads();
    PyObject *p_args = NULL;
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        if (command->max_args == 1) {
            p_args = Py_BuildValue("(O)", Py_BuildValue(""));
            PyObject_CallObject(command->callback, p_args);
            Py_XDECREF(p_args);
        } else {
            PyObject_CallObject(command->callback, p_args);
        }
    } else if (num_args == 1) {
        p_args = Py_BuildValue("(s)", args[0]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 2) {
        p_args = Py_BuildValue("ss", args[0], args[1]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 3) {
        p_args = Py_BuildValue("sss", args[0], args[1], args[2]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 4) {
        p_args = Py_BuildValue("ssss", args[0], args[1], args[2], args[3]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 5) {
        p_args = Py_BuildValue("sssss", args[0], args[1], args[2], args[3], args[4]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    }

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
    allow_python_threads();
}

void
python_timed_callback(PluginTimedFunction *timed_function)
{
    disable_python_threads();
    PyObject_CallObject(timed_function->callback, NULL);
    allow_python_threads();
}

void
python_window_callback(PluginWindowCallback *window_callback, char *tag, char *line)
{
    disable_python_threads();
    PyObject *p_args = NULL;
    p_args = Py_BuildValue("ss", tag, line);
    PyObject_CallObject(window_callback->callback, p_args);
    Py_XDECREF(p_args);

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
    allow_python_threads();
}

static PyMethodDef apiMethods[] = {
    { "cons_alert", python_api_cons_alert, METH_NOARGS, "Highlight the console window in the status bar." },
    { "cons_show", python_api_cons_show, METH_VARARGS, "Print a line to the console." },
    { "cons_show_themed", python_api_cons_show_themed, METH_VARARGS, "Print a themed line to the console" },
    { "cons_bad_cmd_usage", python_api_cons_bad_cmd_usage, METH_VARARGS, "Show invalid command message in console" },
    { "register_command", python_api_register_command, METH_VARARGS, "Register a command." },
    { "register_timed", python_api_register_timed, METH_VARARGS, "Register a timed function." },
    { "completer_add", python_api_completer_add, METH_VARARGS, "Add items to an autocompleter." },
    { "completer_remove", python_api_completer_remove, METH_VARARGS, "Remove items from an autocompleter." },
    { "completer_clear", python_api_completer_clear, METH_VARARGS, "Remove all items from an autocompleter." },
    { "send_line", python_api_send_line, METH_VARARGS, "Send a line of input." },
    { "notify", python_api_notify, METH_VARARGS, "Send desktop notification." },
    { "get_current_recipient", python_api_get_current_recipient, METH_VARARGS, "Return the jid of the recipient of the current window." },
    { "get_current_muc", python_api_get_current_muc, METH_VARARGS, "Return the jid of the room of the current window." },
    { "get_current_nick", python_api_get_current_nick, METH_VARARGS, "Return nickname in current room." },
    { "get_current_occupants", python_api_get_current_occupants, METH_VARARGS, "Return list of occupants in current room." },
    { "current_win_is_console", python_api_current_win_is_console, METH_VARARGS, "Returns whether the current window is the console." },
    { "log_debug", python_api_log_debug, METH_VARARGS, "Log a debug message" },
    { "log_info", python_api_log_info, METH_VARARGS, "Log an info message" },
    { "log_warning", python_api_log_warning, METH_VARARGS, "Log a warning message" },
    { "log_error", python_api_log_error, METH_VARARGS, "Log an error message" },
    { "win_exists", python_api_win_exists, METH_VARARGS, "Determine whether a window exists." },
    { "win_create", python_api_win_create, METH_VARARGS, "Create a new window." },
    { "win_focus", python_api_win_focus, METH_VARARGS, "Focus a window." },
    { "win_show", python_api_win_show, METH_VARARGS, "Show text in the window." },
    { "win_show_themed", python_api_win_show_themed, METH_VARARGS, "Show themed text in the window." },
    { "send_stanza", python_api_send_stanza, METH_VARARGS, "Send an XMPP stanza." },
    { "settings_get_boolean", python_api_settings_get_boolean, METH_VARARGS, "Get a boolean setting." },
    { "settings_set_boolean", python_api_settings_set_boolean, METH_VARARGS, "Set a boolean setting." },
    { "settings_get_string", python_api_settings_get_string, METH_VARARGS, "Get a string setting." },
    { "settings_set_string", python_api_settings_set_string, METH_VARARGS, "Set a string setting." },
    { "settings_get_int", python_api_settings_get_int, METH_VARARGS, "Get a integer setting." },
    { "settings_set_int", python_api_settings_set_int, METH_VARARGS, "Set a integer setting." },
    { "incoming_message", python_api_incoming_message, METH_VARARGS, "Show an incoming message." },
    { "disco_add_feature", python_api_disco_add_feature, METH_VARARGS, "Add a feature to disco info response." },
    { NULL, NULL, 0, NULL }
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef profModule =
{
    PyModuleDef_HEAD_INIT,
    "prof",
    "",
    -1,
    apiMethods
};
#endif

PyMODINIT_FUNC
python_api_init(void)
{
#if PY_MAJOR_VERSION >= 3
    PyObject *result = PyModule_Create(&profModule);
    if (!result) {
        log_debug("Failed to initialise prof module");
    } else {
        log_debug("Initialised prof module");
    }
    return result;
#else
    Py_InitModule("prof", apiMethods);
#endif
}

void
python_init_prof(void)
{
#if PY_MAJOR_VERSION >= 3
    PyImport_AppendInittab("prof", python_api_init);
    Py_Initialize();
    PyEval_InitThreads();
#else
    Py_Initialize();
    PyEval_InitThreads();
    python_api_init();
#endif
}

static char*
_python_plugin_name(void)
{
    PyThreadState *ts = PyThreadState_Get();
    PyFrameObject *frame = ts->frame;
    char const* filename = python_str_or_unicode_to_string(frame->f_code->co_filename);
    gchar **split = g_strsplit(filename, "/", 0);
    char *plugin_name = strdup(split[g_strv_length(split)-1]);
    g_strfreev(split);

    return plugin_name;
}

char*
python_str_or_unicode_to_string(void *obj)
{
    if (!obj) {
        return NULL;
    }
    PyObject *pyobj = (PyObject*)obj;
    if (pyobj == Py_None) {
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(pyobj)) {
        return strdup(PyBytes_AS_STRING(PyUnicode_AsUTF8String(pyobj)));
    } else {
        return strdup(PyBytes_AS_STRING(pyobj));
    }
#else
    if (PyUnicode_Check(pyobj)) {
        return strdup(PyString_AsString(PyUnicode_AsUTF8String(pyobj)));
    } else {
        return strdup(PyString_AsString(pyobj));
    }
#endif
}
