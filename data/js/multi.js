// used when hosting the site on the ESP8266
var address = location.hostname;
var urlBase = "";

// used when hosting the site somewhere other than the ESP8266 (handy for testing without waiting forever to upload to the file system)
// var address = "esp8266-1920f7.local";
// var urlBase = "http://" + address + "/";

// One timer per field, so changing two controls quickly does not drop the first change
var postColorTimer = {};
var postValueTimer = {};

var ignoreColorChange = false;

let controllers = {};

// if used behind HTTPS reverse proxy, replace 'ws' with 'wss' like in the line below
//var ws = new ReconnectingWebSocket('wss://' + address + ':81/', ['arduino']);

var ws = new ReconnectingWebSocket('ws://' + address + ':81/', ['arduino']);
ws.debug = false;

ws.onmessage = function(evt) {
  if(evt.data != null)
  {
    var data = JSON.parse(evt.data);
    if(data == null) return;
    updateFieldValue(data.name, data.value);
  }
}

$(document).ready(function() {
  let subnet = window.location.hostname;

  let n = subnet.lastIndexOf(".");

  subnet = subnet.substr(0, n);

  $("#inputSubnet").val(subnet);

  $("#btnDiscover").click(discover);

  $("#btnSelectAllControllers").click(selectAllControllers);
  $("#btnDeselectAllControllers").click(deselectAllControllers);
});

function selectAllControllers() {
  setAllChecked(true);
}

function deselectAllControllers() {
  setAllChecked(false);
}

function setAllChecked(value) {
  $(".controller-checkbox").prop("checked", value);

  for (let address in controllers) {
    let controller = controllers[address];
    controller.checked = value;
  }
}

function discover() {
  $("#controllersDiv").empty();
  controllers = {};

  let subnet = $("#inputSubnet").val();

  if (!subnet.endsWith(".")) {
    subnet += ".";
  }

  let start = parseInt($("#inputStartAddress").val());
  let end = parseInt($("#inputEndAddress").val());

  let builtForm = false;
  let pending = 0;
  let found = 0;

  if (isNaN(start) || isNaN(end) || start < 1 || end > 254 || start > end) {
    $("#status").html("Invalid IP range, use values from 1 to 254");
    return;
  }

  $("#btnDiscover").prop("disabled", true);
  $("#status").html("Scanning, please wait...");

  for (let i = start; i <= end; i++) {
    pending++;
    let address = subnet + i;

    let url = `http://${address}/`;

    $.ajax({
      url: url + "all",
      success: function(data) {
        let name = "Unknown";

        $.each(data, function(index, field) {
          if (field.name == "name") {
            name = field.value;
          }
        });

        found++;
        console.log(`found controller ${name} at ${address}`);
        let controllerButton = $("#controllerButtonTemplate").clone();
        let checkboxId = "controller-" + address.replace(/\./g, "-");
        controllerButton.removeAttr("id");
        controllerButton.attr("data-address", address);
        var label = controllerButton.find(".controllerLabel");
        var checkbox = controllerButton.find(".controller-checkbox");
        checkbox.attr("id", checkboxId);
        label.attr("for", checkboxId);
        checkbox.attr("data-address", address);
        checkbox.change(controllerCheckboxChange);
        label.text(name + " - " + address);
        $("#controllersDiv").append(controllerButton);

        controllers[address] = {
          address: address,
          checked: true
        };

        $("#hideBeforeDiscover").removeClass("d-none");

        if (!builtForm) {
          builtForm = true;
          $("#form").empty();
          $.each(data, function(index, field) {
            if (field.name == "name") {
            } else if (field.type == "Number") {
              addNumberField(field);
            } else if (field.type == "Boolean") {
              addBooleanField(field);
            } else if (field.type == "Select") {
              addSelectField(field);
            } else if (field.type == "Color") {
              addColorFieldPalette(field);
              addColorFieldPicker(field);
            } else if (field.type == "Section") {
              addSectionField(field);
            } else if (field.type == "String") {
              addStringField(field, false);
            } else if (field.type == "Label") {
              addStringField(field, true);
            }
          });

          $(".minicolors").minicolors({
            theme: "bootstrap",
            changeDelay: 200,
            control: "wheel",
            format: "rgb",
            inline: true
          });
        }
      },
      timeout: 2000
    }).always(function() {
      pending--;

      if (pending > 0) {
        $("#status").html(`Scanning, ${pending} addresses left, found ${found} controller(s)...`);
      } else {
        $("#status").html(`Scan finished, found ${found} controller(s).`);
        $("#btnDiscover").prop("disabled", false);
      }
    });
  }
}

function controllerCheckboxChange() {
  let address = $(this).attr("data-address");
  let controller = controllers[address];
  controller.checked = this.checked;
  console.log(`Controller ${address} selected: ${this.checked}`);
}

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

  $("#form").append(template);
}

function addBooleanField(field) {
  var template = $("#booleanTemplate").clone();

  template.attr("id", "form-group-" + field.name);
  template.attr("data-field-type", field.type);

  var label = template.find(".control-label");
  label.attr("for", "btn-group-" + field.name);
  label.text(field.label);

  var btngroup = template.find(".btn-group");
  btngroup.attr("id", "btn-group-" + field.name);

  var btnOn = template.find("#btnOn");
  var btnOff = template.find("#btnOff");

  btnOn.attr("id", "btnOn" + field.name);
  btnOff.attr("id", "btnOff" + field.name);

  btnOn.attr("class", field.value ? "btn btn-primary" : "btn btn-outline-primary");
  btnOff.attr("class", !field.value ? "btn btn-primary" : "btn btn-outline-primary");

  btnOn.click(function() {
    setBooleanFieldValue(field, btnOn, btnOff, 1)
  });
  btnOff.click(function() {
    setBooleanFieldValue(field, btnOn, btnOff, 0)
  });

  $("#form").append(template);
}

function toggleColorSelector(pattern) {
  var patterns = [
    'Solid Color'
  ];
  if (patterns.indexOf(pattern) > -1) {
    $('#form-group-solidColor-palette').show();
    $('#form-group-solidColor').show();
    $('#form-group-section-solidColor').hide();
  } else {
    $('#form-group-solidColor-palette').hide();
    $('#form-group-solidColor').hide();
    $('#form-group-section-solidColor').hide();
  }
}

function toggleCoolingSelector(pattern) {
  var coolingPatterns = [
    'Fire',
    'Water'
  ];
  if (coolingPatterns.indexOf(pattern) > -1) {
    $('#form-group-section-fire').show();
    $('#form-group-cooling').show();
    $('#form-group-sparking').show();
  } else {
    $('#form-group-section-fire').hide();
    $('#form-group-cooling').hide();
    $('#form-group-sparking').hide();
  }
}

function toggleTwinkleFadeSelector(pattern) {
  var twinkleFadePatterns =
    [
      "Incandescent Twinkles",
      "Cloud Twinkles",
      "Snow Twinkles",
      "Rainbow Twinkles",
    ];
  if (twinkleFadePatterns.indexOf(pattern) > -1) {
    $('#form-group-section-fades').show();
    $('#form-group-fadeInSpeed').show();
    $('#form-group-fadeOutSpeed').show();

  } else {
    $('#form-group-section-fades').hide();
    $('#form-group-fadeInSpeed').hide();
    $('#form-group-fadeOutSpeed').hide();
  }
}

function toggleTwinkleSelector(pattern) {
  var twinklePatterns =
    [
      "Retro C9 Twinkles",
      "Red & White Twinkles",
      "Blue & White Twinkles",
      "Blue & Incandescent Twinkles",
      "Red & Green & Blue Twinkles",
      "Red, Green & White Twinkles",
      "Fairy Light Twinkles",
      "Snow 2 Twinkles",
      "Holly Twinkles",
      "Ice Twinkles",
      "Party Twinkles",
      "Easter Twinkles",
      "Forest Twinkles",
      "Lava Twinkles",
      "Fire Twinkles",
      "Cloud 2 Twinkles",
      "Ocean Twinkles",
    ];
  if (twinklePatterns.indexOf(pattern) > -1) {
    $('#form-group-section-twinkles').show();
    $('#form-group-twinkleSpeed').show();
    $('#form-group-twinkleDensity').show();
  } else {
    $('#form-group-section-twinkles').hide();
    $('#form-group-twinkleSpeed').hide();
    $('#form-group-twinkleDensity').hide();
  }
}

function toggleSpeedSelector(pattern) {
  var speedPatterns =
    [
      "Sinelon",
      "Beat",
      "Confetti",
      "White Sparkles",
      "Rainbow",
      "Rainbow With Glitter"
    ];
  if (speedPatterns.indexOf(pattern) > -1) {
    $('#form-group-speed').show();

  } else {
    $('#form-group-speed').hide();
  }
}

function togglePalletteSelector(pattern) {
  var pallettePatterns =
    [
      "Confetti",
      "Sinelon",
      "Beat",
      "Midnight Lake",
      "Colorful Meteor Rain"
    ];
  if (pallettePatterns.indexOf(pattern) > -1) {
    $('#form-group-palette').show();

  } else {
    $('#form-group-palette').hide();
  }
}

function toggleGradientPalletteSelector(pattern) {
  var gradientPallettePatterns =
    [
      "Gradient Sweep"
    ];
    
  if (gradientPallettePatterns.indexOf(pattern) > -1) {
    $('#form-group-gradientPalette').show();

  } else {
    $('#form-group-gradientPalette').hide();
  }
}

function toggleSections(pattern) {
  toggleColorSelector(pattern);
  toggleCoolingSelector(pattern);
  toggleTwinkleFadeSelector(pattern);
  toggleTwinkleSelector(pattern);
  toggleSpeedSelector(pattern);
  togglePalletteSelector(pattern);
  toggleGradientPalletteSelector(pattern);
}

$.fn.toggleSections = function (pattern) {
  toggleSections(pattern);
}

function addSelectField(field) {
  var template = $("#selectTemplate").clone();

  template.attr("id", "form-group-" + field.name);
  template.attr("data-field-type", field.type);

  var id = "input-" + field.name;

  var label = template.find(".control-label");
  label.attr("for", id);
  label.text(field.label);

  var select = template.find("select");
  select.attr("id", id);

  for (var i = 0; i < field.options.length; i++) {
    var optionText = field.options[i];
    var option = $("<option></option>");
    option.text(optionText);
    option.attr("value", i);
    select.append(option);
  }

  select.val(field.value);

  select.change(function() {
    var value = template.find("#" + id + " option:selected").index();
    postValue(field.name, value);

    if (id == 'input-pattern') {
      toggleSections(field.options[select.val()]);
    }
  });

  var previousButton = template.find(".btn-previous");
  var nextButton = template.find(".btn-next");

  previousButton.click(function() {
    var value = template.find("#" + id + " option:selected").index();
    var count = select.find("option").length;
    value--;
    if(value < 0)
      value = count - 1;
    select.val(value);
    postValue(field.name, value);
    if (id == 'input-pattern') {
      toggleSections(field.options[select.val()]);
    }
  });

  nextButton.click(function() {
    var value = template.find("#" + id + " option:selected").index();
    var count = select.find("option").length;
    value++;
    if(value >= count)
      value = 0;
    select.val(value);
    postValue(field.name, value);
    if (id == 'input-pattern') {
      toggleSections(field.options[select.val()]);
    }
  });

  $("#form").append(template).toggleSections(field.options[select.val()]);
}

function addColorFieldPicker(field) {
  var template = $("#colorTemplate").clone();

  template.attr("id", "form-group-" + field.name);
  template.attr("data-field-type", field.type);

  var id = "input-" + field.name;

  var input = template.find(".minicolors");
  input.attr("id", id);

  if(!field.value.startsWith("rgb("))
    field.value = "rgb(" + field.value;

  if(!field.value.endsWith(")"))
    field.value += ")";

  input.val(field.value);

  var components = rgbToComponents(field.value);

  var redInput = template.find(".color-red-input");
  var greenInput = template.find(".color-green-input");
  var blueInput = template.find(".color-blue-input");

  var redSlider = template.find(".color-red-slider");
  var greenSlider = template.find(".color-green-slider");
  var blueSlider = template.find(".color-blue-slider");

  redInput.attr("id", id + "-red");
  greenInput.attr("id", id + "-green");
  blueInput.attr("id", id + "-blue");

  redSlider.attr("id", id + "-red-slider");
  greenSlider.attr("id", id + "-green-slider");
  blueSlider.attr("id", id + "-blue-slider");

  redInput.val(components.r);
  greenInput.val(components.g);
  blueInput.val(components.b);

  redSlider.val(components.r);
  greenSlider.val(components.g);
  blueSlider.val(components.b);

  redInput.on("change", function() {
    var value = $("#" + id).val();
    var r = $(this).val();
    var components = rgbToComponents(value);
    field.value = r + "," + components.g + "," + components.b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    redSlider.val(r);
  });

  greenInput.on("change", function() {
    var value = $("#" + id).val();
    var g = $(this).val();
    var components = rgbToComponents(value);
    field.value = components.r + "," + g + "," + components.b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    greenSlider.val(g);
  });

  blueInput.on("change", function() {
    var value = $("#" + id).val();
    var b = $(this).val();
    var components = rgbToComponents(value);
    field.value = components.r + "," + components.g + "," + b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    blueSlider.val(b);
  });

  redSlider.on("change", function() {
    var value = $("#" + id).val();
    var r = $(this).val();
    var components = rgbToComponents(value);
    field.value = r + "," + components.g + "," + components.b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    redInput.val(r);
  });

  greenSlider.on("change", function() {
    var value = $("#" + id).val();
    var g = $(this).val();
    var components = rgbToComponents(value);
    field.value = components.r + "," + g + "," + components.b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    greenInput.val(g);
  });

  blueSlider.on("change", function() {
    var value = $("#" + id).val();
    var b = $(this).val();
    var components = rgbToComponents(value);
    field.value = components.r + "," + components.g + "," + b;
    $("#" + id).minicolors("value", "rgb(" + field.value + ")");
    blueInput.val(b);
  });

  redSlider.on("input", function() {
    redInput.val($(this).val());
  });

  greenSlider.on("input", function() {
    greenInput.val($(this).val());
  });

  blueSlider.on("input", function() {
    blueInput.val($(this).val());
  });

  input.on("change", function() {
    if (ignoreColorChange) return;

    var value = $(this).val();
    var components = rgbToComponents(value);

    // Picker was moved to a color received from the controller, do not send it back
    if ($(this).data("serverValue") == components.r + "," + components.g + "," + components.b) return;

    redInput.val(components.r);
    greenInput.val(components.g);
    blueInput.val(components.b);

    redSlider.val(components.r);
    greenSlider.val(components.g);
    blueSlider.val(components.b);

    field.value = components.r + "," + components.g + "," + components.b;
    delayPostColor(field.name, components);
  });

  $("#form").append(template);

  setTimeout(function () {
  toggleSections($("#input-pattern option:selected").text());
  }, 0);
}

function addColorFieldPalette(field) {
  var template = $("#colorPaletteTemplate").clone();

  template.attr("id", "form-group-" + field.name + "-palette");

  var buttons = template.find(".btn-color");

  var label = template.find(".control-label");
  label.text(field.label);

  buttons.each(function(index, button) {
    $(button).click(function() {
      var rgb = $(this).css('backgroundColor');
      var components = rgbToComponents(rgb);

      field.value = components.r + "," + components.g + "," + components.b;
      postColor(field.name, components);

      ignoreColorChange = true;
      var id = "#input-" + field.name;
      $(id).minicolors("value", "rgb(" + field.value + ")");
      $(id + "-red").val(components.r);
      $(id + "-green").val(components.g);
      $(id + "-blue").val(components.b);
      $(id + "-red-slider").val(components.r);
      $(id + "-green-slider").val(components.g);
      $(id + "-blue-slider").val(components.b);
      ignoreColorChange = false;
    });
  });

  $("#form").append(template);
}

function addSectionField(field) {
  var template = $("#sectionTemplate").clone();

  template.attr("id", "form-group-section-" + field.name);
  template.attr("data-field-type", field.type);

  $("#form").append(template);
}

function addStringField(field, readonly) {
  var template;

  if (readonly) {
    template = $("#labelTemplate").clone();
  } else {
    template = $("#stringTemplate").clone();
  }

  template.attr("id", "form-group-" + field.name);
  template.attr("data-field-type", field.type);

  var label = template.find(".control-label");
  label.attr("for", "input-" + field.name);
  label.text(field.label);

  var input = template.find(".input");
  input.val(field.value);

  if (!readonly) {
    input.on("change", function() {
      var value = $(this).val();
      field.value = value;
      delayPostValue(field.name, value);
    });
  }

  $("#form").append(template);
}

function updateFieldValue(name, value) {
  // Controller broadcasts solid color changes as "color"
  if (name == "color") {
    name = "solidColor";
  }

  var group = $("#form-group-" + name);

  var type = group.attr("data-field-type");

  if (type == "Number") {
    group.find(".input, .slider").val(toDisplayValue(name, value));
  } else if (type == "Boolean") {
    var btnOn = group.find("#btnOn" + name);
    var btnOff = group.find("#btnOff" + name);

    btnOn.attr("class", value ? "btn btn-primary" : "btn btn-outline-primary");
    btnOff.attr("class", !value ? "btn btn-primary" : "btn btn-outline-primary");

  } else if (type == "Select") {
    var select = group.find("select");
    select.val(value);

    if (name == "pattern") {
      toggleSections(select.find("option:selected").text());
    }
  } else if (type == "Color") {
    var components = rgbToComponents("rgb(" + value + ")");
    var id = "#input-" + name;

    $(id).data("serverValue", value);
    $(id).minicolors("value", "rgb(" + value + ")");
    $(id + "-red").val(components.r);
    $(id + "-green").val(components.g);
    $(id + "-blue").val(components.b);
    $(id + "-red-slider").val(components.r);
    $(id + "-green-slider").val(components.g);
    $(id + "-blue-slider").val(components.b);
  }
}

function setBooleanFieldValue(field, btnOn, btnOff, value) {
  field.value = value;

  btnOn.attr("class", field.value ? "btn btn-primary" : "btn btn-outline-primary");
  btnOff.attr("class", !field.value ? "btn btn-primary" : "btn btn-outline-primary");

  postValue(field.name, field.value);
}

function postValue(name, value) {
  var body = { name: name, value: value };

  for (let address in controllers) {
    try {
      let controller = controllers[address];
      let checked = controller.checked;

      let url = `http://${address}/`;

      if (!checked) {
        continue;
      }

      url = url + name + "?value=" + value;

      console.log(url);

      $("#status").html("Setting " + name + ": " + formatFieldValue(name, value) + ", please wait...");

      $.post(url, body, function(data) {
        if (data.name != null) {
          $("#status").html("Set " + name + ": " + data.name);
        } else {
          $("#status").html("Set " + name + ": " + formatFieldValue(name, data));
        }
      })
      .fail(function() { $("#status").html("Fail: " + name + " on " + address); });
    } catch (error) {
      console.error(error);
    }
  }
}

function delayPostValue(name, value) {
  clearTimeout(postValueTimer[name]);
  postValueTimer[name] = setTimeout(function() {
    postValue(name, value);
  }, 300);
}

function postColor(name, value) {
  var body = { name: name, r: value.r, g: value.g, b: value.b };

  for (let address in controllers) {
    try {
      let controller = controllers[address];
      let checked = controller.checked;

      let url = `http://${address}`;

      if (!checked) {
        continue;
      }

      url = `${url}/${name}?r=${value.r}&g=${value.g}&b=${value.b}`;

      console.log(url);

      $("#status").html(
        `Setting ${name}: ${value.r},${value.g},${value.b}, please wait...`
      );

      $.post(url, body, function(data) {
        if (data.name != null) {
          $("#status").html("Set " + name + ": " + data.name);
        } else {
          $("#status").html("Set " + name + ": " + formatFieldValue(name, data));
        }
      })
      .fail(function() { $("#status").html("Fail: " + name + " on " + address); });
    } catch (error) {
      console.error(error);
    }
  }
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
