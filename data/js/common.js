// Shared by all pages: color theme and how field values are shown to the user.
// Loaded in <head>, so the theme is set before the page is drawn.

// Dark theme is the default, the choice made with the navigation bar button is remembered in this browser
(function () {
  var theme = "dark";

  try {
    theme = localStorage.getItem("theme") || "dark";
  } catch (e) {
    // Storage blocked (private mode), keep the default
  }

  document.documentElement.setAttribute("data-bs-theme", theme);
})();

function setTheme(theme) {
  document.documentElement.setAttribute("data-bs-theme", theme);

  try {
    localStorage.setItem("theme", theme);
  } catch (e) {
    // Storage blocked, theme only lasts until the page is reloaded
  }
}

document.addEventListener("DOMContentLoaded", function () {
  var button = document.getElementById("btnTheme");

  if (button) {
    button.addEventListener("click", function () {
      setTheme(document.documentElement.getAttribute("data-bs-theme") == "dark" ? "light" : "dark");
    });
  }
});

// Fields shown to the user in other units than the controller uses.
// The controller keeps brightness as 0-255, it is shown as 0-100 %. Autoplay duration is in seconds.
var fieldUnits = {
  brightness: {
    unit: "%",
    min: 0,
    max: 100,
    toDisplay: function (value) { return Math.round(value * 100 / 255); },
    toValue: function (percent) { return Math.round(percent * 255 / 100); }
  },
  autoplayDuration: {
    unit: "s"
  }
};

function fieldUnit(name) {
  return fieldUnits[name] || {};
}

// Controller value -> value shown in the page
function toDisplayValue(name, value) {
  var unit = fieldUnit(name);
  return unit.toDisplay ? unit.toDisplay(Number(value)) : value;
}

// Value entered in the page -> controller value
function toFieldValue(name, value) {
  var unit = fieldUnit(name);
  return unit.toValue ? unit.toValue(Number(value)) : value;
}

// Controller value as text with its unit, for status messages
function formatFieldValue(name, value) {
  var unit = fieldUnit(name).unit;

  if (!unit) {
    return value;
  }

  return toDisplayValue(name, value) + (unit == "%" ? "%" : " " + unit);
}
