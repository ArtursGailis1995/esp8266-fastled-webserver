// used when hosting the site on the ESP8266
var address = location.hostname;
var urlBase = "";

// used when hosting the site somewhere other than the ESP8266 (handy for testing without waiting forever to upload to the file system)
// var address = "192.168.1.13";
// var urlBase = "http://" + address + "/";

// One timer per field, so changing two controls quickly does not drop the first change
var postColorTimer = {};
var postValueTimer = {};

var ignoreColorChange = false;

var patterns = [
  "Dynamic Rainbow",
  "Color Waves",
  "Gradient Sweep",
  "Gradient Showcase",

  "Rainbow Twinkles",
  "Snow Twinkles",
  "Cloud Twinkles",
  "Incandescent Twinkles",

  "Retro C9 Twinkles",
  "Red & White Twinkles",
  "Blue & White Twinkles",
  "Blue & Incandescent Twinkles",
  "Red & Green & Blue Twinkles",
  "Red, Green & White Twinkles",
  "Fairy Light Twinkles",
  "Easter Twinkles",
  "Snow 2 Twinkles",
  "Holly Twinkles",
  "Ice Twinkles",
  "Party Twinkles",
  "Forest Twinkles",
  "Lava Twinkles",
  "Fire Twinkles",
  "Cloud 2 Twinkles",
  "Ocean Twinkles",

  "Rainbow",
  "Rainbow With Glitter",
  "White Sparkles",
  "Solid Rainbow",
  "Confetti",
  "Sinelon",
  "Beat",
  "Juggle",
  "Fire",
  "Water",
  "Pacifica Loop",
  "Midnight Lake",
  "Meteor Rain",
  "Colorful Meteor Rain",
  "Plasma"
];

// if used behind HTTPS reverse proxy, replace 'ws' with 'wss' like in the line below
//var ws = new ReconnectingWebSocket('wss://' + address + ':81/', ['arduino']);

var ws = new ReconnectingWebSocket('ws://' + address + ':81/', ['arduino']);
ws.debug = false;

ws.onmessage = function(evt) {
  if(evt.data != null)
  {
    var data = JSON.parse(evt.data);
    if(data == null) return;
    switch(data.name) {
      case "power":
        if(data.value == 1) {
          $("#btnOn").attr("class", "btn btn-primary");
          $("#btnOff").attr("class", "btn btn-outline-primary");
        } else {
          $("#btnOff").attr("class", "btn btn-primary");
          $("#btnOn").attr("class", "btn btn-outline-primary");
        }
        break;

      case "pattern":
        $(".grid-item-pattern").attr("class", "grid-item-pattern btn btn-outline-primary");
        $("#pattern-button-" + data.value).attr("class", "grid-item-pattern btn btn-primary");
        break;

      case "brightness":
        $("#form-group-brightness .input, #form-group-brightness .slider").val(toDisplayValue("brightness", data.value));
        break;
    }
  }
}

$(document).ready(function() {
  $("#status").html("Connecting, please wait...");

  $.get(urlBase + "all", function(data) {
    $("#status").html("Loading, please wait...");

    $.each(data, function(index, field) {
      switch (field.name) {
        case "power":
          if(field.value == 1) {
            $("#btnOn").attr("class", "btn btn-primary");
          } else {
            $("#btnOff").attr("class", "btn btn-primary");
          }
          break;

        case "pattern":
          addPatternButtons(field);
          break;

        case "brightness":
          addNumberField(field);
          break;
      }
    });
  });

  addColorButtons();

  $("#btnOn").click(function() {
    postValue("power", 1);
    $("#btnOn").attr("class", "btn btn-primary");
    $("#btnOff").attr("class", "btn btn-outline-primary");
  });

  $("#btnOff").click(function() {
    postValue("power", 0);
    $("#btnOff").attr("class", "btn btn-primary");
    $("#btnOn").attr("class", "btn btn-outline-primary");
  });

  $("#status").html("Ready");
});

function addNumberField(field) {
  var template = $("#numberTemplate").clone();

  template.attr("id", "form-group-" + field.name);
  template.attr("data-field-type", field.type);

  var label = template.find(".control-label");
  label.attr("for", "input-" + field.name);
  label.text(field.label);

  var input = template.find(".input");
  var slider = template.find(".slider");
  slider.attr("id", "input-" + field.name);
  // Some fields are shown in other units than the controller uses, see common.js
  var unit = fieldUnit(field.name);
  var min = unit.min !== undefined ? unit.min : field.min;
  var max = unit.max !== undefined ? unit.max : field.max;

  if (min !== undefined) {
    input.attr("min", min);
    slider.attr("min", min);
  }
  if (max !== undefined) {
    input.attr("max", max);
    slider.attr("max", max);
  }
  if (field.step) {
    input.attr("step", field.step);
    slider.attr("step", field.step);
  }
  if (unit.unit) {
    template.find(".unit").text(unit.unit).removeClass("d-none");
  }

  input.val(toDisplayValue(field.name, field.value));
  slider.val(toDisplayValue(field.name, field.value));

  function setValue(value) {
    value = Math.min(Number(input.attr("max")), Math.max(Number(input.attr("min")), Number(value)));
    input.val(value);
    slider.val(value);
    field.value = toFieldValue(field.name, value);
    delayPostValue(field.name, field.value);
  }

  slider.on("input", function() {
    input.val($(this).val());
  });

  slider.on("change", function() {
    setValue($(this).val());
  });

  input.on("change", function() {
    setValue($(this).val());
  });

  $("#brightnessSliderRow").append(template);
}

function addColorButtons() {
  var hues = 25;
  var hueStep = 360 / hues;

  var levels = 10;
  var levelStep = 60 / levels;

  for(var l = 20; l < 80; l += levelStep) {
    for(var h = 0; h < hues; h++) {
      addColorButton(h * hueStep, 100, l);
    }
  }

}

var colorButtonIndex = 0;

function addColorButton(h, s, l) {
  var color = "hsla(" + h + ", " + s + "%, " + l + "%, 1)"
  var template = $("#colorButtonTemplate").clone();
  template.attr("id", "color-button-" + colorButtonIndex++);
  template.css("background-color", color);
  template.click(function() {
    var rgb = $(this).css('backgroundColor');
    var components = rgbToComponents(rgb);

    $(".grid-item-color").removeClass("selected");
    $(this).addClass("selected");

    postColor("solidColor", components);
  });

  $("#colorButtonsRow").append(template);
}

function addPatternButtons(patternField) {
  $.each(patternField.options, function(index, pattern) {
    if($.inArray(pattern, patterns) == -1)
      return;

    var template = $("#patternButtonTemplate").clone();
    template.attr("id", "pattern-button-" + index);
    template.text(pattern);
    template.click(function() {
      postValue("patternName", pattern);
      $(".grid-item-color").removeClass("selected");
      $(".grid-item-pattern").attr("class", "grid-item-pattern btn btn-outline-primary");
      $(this).attr("class", "grid-item-pattern btn btn-primary");
    });

    $("#patternGrid").append(template);
  });

  $("#pattern-button-" + patternField.value).attr("class", "grid-item-pattern btn btn-primary");
}

function postValue(name, value) {
  $("#status").html("Setting " + name + ": " + formatFieldValue(name, value) + ", please wait...");

  var body = { name: name, value: value };

  $.post(urlBase + name, body, function(data) {
    if (data.name != null) {
      $("#status").html("Set " + name + ": " + data.name);
    } else {
      $("#status").html("Set " + name + ": " + formatFieldValue(name, data));
    }
  })
  .fail(function(jqXHR, textStatus) { console.log("Fail: " + name + " " + textStatus); });
}

function delayPostValue(name, value) {
  clearTimeout(postValueTimer[name]);
  postValueTimer[name] = setTimeout(function() {
    postValue(name, value);
  }, 300);
}

function postColor(name, value) {
  $("#status").html("Setting " + name + ": " + value.r + "," + value.g + "," + value.b + ", please wait...");

  var body = { name: name, r: value.r, g: value.g, b: value.b };

  $.post(urlBase + name + "?r=" + value.r + "&g=" + value.g + "&b=" + value.b, body, function(data) {
    $("#status").html("Set " + name + ": " + formatFieldValue(name, data));
  })
  .fail(function(textStatus, errorThrown) { $("#status").html("Fail: " + textStatus + " " + errorThrown); });
}

function delayPostColor(name, value) {
  clearTimeout(postColorTimer[name]);
  postColorTimer[name] = setTimeout(function() {
    postColor(name, value);
  }, 300);
}

function componentToHex(c) {
  var hex = c.toString(16);
  return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
  return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

function rgbToComponents(rgb) {
  var components = {};

  rgb = rgb.match(/^rgb\((\d+),\s*(\d+),\s*(\d+)\)$/);
  components.r = parseInt(rgb[1]);
  components.g = parseInt(rgb[2]);
  components.b = parseInt(rgb[3]);

  return components;
}
