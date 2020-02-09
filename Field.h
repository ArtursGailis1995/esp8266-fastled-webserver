/*
   ESP8266 + FastLED + IR Remote: https://github.com/jasoncoon/esp8266-fastled-webserver
   Copyright (C) 2016 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

typedef String (*FieldSetter)(String);
typedef String (*FieldGetter)();

const String NumberFieldType = "Number";
const String BooleanFieldType = "Boolean";
const String SelectFieldType = "Select";
const String ColorFieldType = "Color";
const String SectionFieldType = "Section";

typedef struct Field {
	String name;
	String label;
	String type;
	uint8_t min;
	uint8_t max;
	FieldGetter getValue;
	FieldGetter getOptions;
	FieldSetter setValue;
};

typedef Field FieldList[];

Field getField(String name, FieldList fields, uint8_t count) {
	for (uint8_t i = 0; i < count; i++) {
		Field field = fields[i];

		if (field.name == name) {
			return field;
		}
	}
	return Field();
}

String getFieldValue(String name, FieldList fields, uint8_t count) {
	Field field = getField(name, fields, count);

	if (field.getValue) {
		return field.getValue();
	}

	return String();
}

String setFieldValue(String name, String value, FieldList fields, uint8_t count) {
	Field field = getField(name, fields, count);

	if (field.setValue) {
		return field.setValue(value);
	}

	return String();
}

String getFieldsJson(FieldList fields, uint8_t count) {
	String json = "[";

	for (uint8_t i = 0; i < count; i++) {
		Field field = fields[i];

		json += "{\"name\":\"" + field.name + "\",\"label\":\"" + field.label + "\",\"type\":\"" + field.type + "\"";

		if(field.getValue) {
			if (field.type == ColorFieldType || field.type == "String") {
				json += ",\"value\":\"" + field.getValue() + "\"";
			}
			else {
				json += ",\"value\":" + field.getValue();
			}
		}

		if (field.type == NumberFieldType) {
			json += ",\"min\":" + String(field.min);
			json += ",\"max\":" + String(field.max);
		}

		if (field.getOptions) {
			json += ",\"options\":[";
			json += field.getOptions();
			json += "]";
		}

		json += "}";

		if (i < count - 1)
			json += ",";
	}

	json += "]";

	return json;
}
