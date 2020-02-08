function createFileUploader(d, f, g) {
	var h;
	var i = document.createElement("input");
	i.type = "file";
	i.multiple = false;
	i.name = "data";
	document.getElementById(d).appendChild(i);
	var j = document.createElement("input");
	j.id = "upload-path";
	j.type = "text";
	j.name = "path";
	j.defaultValue = "/";
	document.getElementById(d).appendChild(j);
	var k = document.createElement("button");
	k.innerHTML = 'Upload';
	document.getElementById(d).appendChild(k);
	var l = document.createElement("button");
	l.innerHTML = 'Create';
	document.getElementById(d).appendChild(l);

	function httpPostProcessRequest() {
		if (h.readyState == 4) {
			if (h.status != 200) alert("ERROR[" + h.status + "]: " + h.responseText);
			else {
				f.refreshPath(j.value)
			}
		}
	}

	function createPath(p) {
		h = new XMLHttpRequest();
		h.onreadystatechange = httpPostProcessRequest;
		var a = new FormData();
		a.append("path", p);
		h.open("PUT", "/edit");
		h.send(a)
	}

	l.onclick = function (e) {
		if (j.value.indexOf(".") === -1) return;
		createPath(j.value);
		g.loadUrl(j.value)
	};

	k.onclick = function (e) {
		if (i.files.length === 0) {
			return
		}
		h = new XMLHttpRequest();
		h.onreadystatechange = httpPostProcessRequest;
		var a = new FormData();
		a.append("data", i.files[0], j.value);
		h.open("POST", "/edit");
		h.send(a)
	};

	i.onchange = function (e) {
		if (i.files.length === 0) return;
		var a = i.files[0].name;
		var b = /(?:\.([^.]+))?$/.exec(a)[1];
		var c = /(.*)\.[^.]+$/.exec(a)[1];
		if (typeof c !== undefined) {
			a = c
		}
		if (typeof b !== undefined) {
			if (b === "html") b = "htm";
			else if (b === "jpeg") b = "jpg";
			a = a + "." + b
		}
		if (j.value === "/" || j.value.lastIndexOf("/") === 0) {
			j.value = "/" + a
		} else {
			j.value = j.value.substring(0, j.value.lastIndexOf("/") + 1) + a
		}
	}
}

function createTree(k, l) {
	var m = document.getElementById("preview");
	var n = document.createElement("div");
	n.className = "tvu";
	document.getElementById(k).appendChild(n);

	function loadDownload(a) {
		document.getElementById('download-frame').src = a + "?download=true"
	}

	function loadPreview(a) {
		document.getElementById("editor").style.display = "none";
		m.style.display = "block";
		m.innerHTML = '<img src="' + a + '?_cb=' + Date.now() + '" style="max-width:100%; max-height:100%; margin:auto; display:block;" />'
	}

	function fillFileMenu(a, b) {
		var c = document.createElement("ul");
		a.appendChild(c);
		var d = document.createElement("li");
		c.appendChild(d);

		if (isTextFile(b)) {
			d.innerHTML = "<span>Edit</span>";
			d.onclick = function (e) {
				l.loadUrl(b);
				if (document.body.getElementsByClassName('cm').length > 0) document.body.removeChild(a)
			}
		} else if (isImageFile(b)) {
			d.innerHTML = "<span>Preview</span>";
			d.onclick = function (e) {
				loadPreview(b);
				if (document.body.getElementsByClassName('cm').length > 0) document.body.removeChild(a)
			}
		}

		var f = document.createElement("li");
		c.appendChild(f);
		f.innerHTML = "<span>Download</span>";

		f.onclick = function (e) {
			loadDownload(b);
			if (document.body.getElementsByClassName('cm').length > 0) document.body.removeChild(a)
		};

		var g = document.createElement("li");
		c.appendChild(g);
		g.innerHTML = "<span>Delete</span>";

		g.onclick = function (e) {
			httpDelete(b);
			if (document.body.getElementsByClassName('cm').length > 0) document.body.removeChild(a)
		}
	}

	function showContextMenu(e, a, b) {
		var c = document.createElement("div");
		var d = document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
		var f = document.body.scrollLeft ? document.body.scrollLeft : document.documentElement.scrollLeft;
		var g = event.clientX + f;
		var h = event.clientY + d;
		c.className = 'cm';
		c.style.display = 'block';
		c.style.left = g + 'px';
		c.style.top = h + 'px';
		fillFileMenu(c, a);
		document.body.appendChild(c);
		var i = c.offsetWidth;
		var j = c.offsetHeight;

		c.onmouseout = function (e) {
			if (e.clientX < g || e.clientX > (g + i) || e.clientY < h || e.clientY > (h + j)) {
				if (document.body.getElementsByClassName('cm').length > 0) document.body.removeChild(c)
			}
		}
	}

	function createTreeLeaf(a, b, c) {
		var d = document.createElement("li");
		d.id = (((a == "/") ? "" : a) + "/" + b);
		var f = document.createElement("span");
		f.innerText = b;
		d.appendChild(f);

		d.onclick = function (e) {
			if (isTextFile(d.id.toLowerCase())) {
				l.loadUrl(d.id)
			} else if (isImageFile(d.id.toLowerCase())) {
				loadPreview(d.id)
			}
		};

		d.oncontextmenu = function (e) {
			e.preventDefault();
			e.stopPropagation();
			showContextMenu(e, d.id, true)
		};
		return d
	}

	function addList(a, b, c) {
		var d = document.createElement("ul");
		a.appendChild(d);
		var e = c.length;
		for (var i = 0; i < e; i++) {
			if (c[i].type === "file") d.appendChild(createTreeLeaf(b, c[i].name, c[i].size))
		}
	}

	function isTextFile(a) {
		var b = /(?:\.([^.]+))?$/.exec(a)[1];
		if (typeof b !== undefined) {
			switch (b) {
				case "txt":
				case "htm":
				case "js":
				case "c":
				case "cpp":
				case "css":
				case "xml":
					return true
			}
		}
		return false
	}

	function isImageFile(a) {
		var b = /(?:\.([^.]+))?$/.exec(a)[1];
		if (typeof b !== undefined) {
			switch (b) {
				case "png":
				case "jpg":
				case "gif":
					return true
			}
		}
		return false
	}

	this.refreshPath = function (a) {
		n.removeChild(n.childNodes[0]);
		httpGet(n, "/")
	};

	function delCb(a) {
		return function () {
			if (xmlHttp.readyState == 4) {
				if (xmlHttp.status != 200) {
					alert("ERROR[" + xmlHttp.status + "]: " + xmlHttp.responseText)
				} else {
					n.removeChild(n.childNodes[0]);
					httpGet(n, "/")
				}
			}
		}
	}

	function httpDelete(a) {
		xmlHttp = new XMLHttpRequest();
		xmlHttp.onreadystatechange = delCb(a);
		var b = new FormData();
		b.append("path", a);
		xmlHttp.open("DELETE", "/edit");
		xmlHttp.send(b)
	}

	function getCb(a, b) {
		return function () {
			if (xmlHttp.readyState == 4) {
				if (xmlHttp.status == 200) addList(a, b, JSON.parse(xmlHttp.responseText))
			}
		}
	}

	function httpGet(a, b) {
		xmlHttp = new XMLHttpRequest(a, b);
		xmlHttp.onreadystatechange = getCb(a, b);
		xmlHttp.open("GET", "/list?dir=" + b, true);
		xmlHttp.send(null)
	}

	httpGet(n, "/");
	return this
}

function createEditor(e, f, g, h, i) {
	function getLangFromFilename(a) {
		var b = "plain";
		var c = /(?:\.([^.]+))?$/.exec(a)[1];
		if (typeof c !== undefined) {
			switch (c) {
				case "txt":
					b = "plain";
					break;
				case "htm":
					b = "html";
					break;
				case "js":
					b = "javascript";
					break;
				case "c":
					b = "c_cpp";
					break;
				case "cpp":
					b = "c_cpp";
					break;
				case "css":
				case "scss":
				case "php":
				case "html":
				case "json":
				case "xml":
					b = c
			}
		}
		return b
	}

	if (typeof f === "undefined") f = "/index.htm";
	if (typeof g === "undefined") {
		g = getLangFromFilename(f)
	}

	if (typeof h === "undefined") h = "textmate";
	if (typeof i === "undefined") {
		i = "text/" + g;
		if (g === "c_cpp") i = "text/plain"
	}

	var j = null;
	var k = ace.edit(e);

	function httpPostProcessRequest() {
		if (j.readyState == 4) {
			if (j.status != 200) alert("ERROR[" + j.status + "]: " + j.responseText)
		}
	}

	function httpPost(a, b, c) {
		j = new XMLHttpRequest();
		j.onreadystatechange = httpPostProcessRequest;
		var d = new FormData();
		d.append("data", new Blob([b], {
			type: c
		}), a);
		j.open("POST", "/edit");
		j.send(d)
	}

	function httpGetProcessRequest() {
		if (j.readyState == 4) {
			document.getElementById("preview").style.display = "none";
			document.getElementById("editor").style.display = "block";
			if (j.status == 200) k.setValue(j.responseText);
			else k.setValue("");
			k.clearSelection()
		}
	}

	function httpGet(a) {
		j = new XMLHttpRequest();
		j.onreadystatechange = httpGetProcessRequest;
		j.open("GET", a, true);
		j.send(null)
	}

	if (g !== "plain") k.getSession().setMode("ace/mode/" + g);

	k.setTheme("ace/theme/" + h);
	k.$blockScrolling = Infinity;
	k.getSession().setUseSoftTabs(true);
	k.getSession().setTabSize(2);
	k.setHighlightActiveLine(true);
	k.setShowPrintMargin(false);

	k.commands.addCommand({
		name: 'saveCommand',
		bindKey: {
			win: 'Ctrl-S',
			mac: 'Command-S'
		},
		exec: function (a) {
			httpPost(f, a.getValue() + "", i)
		},
		readOnly: false
	});

	k.commands.addCommand({
		name: 'undoCommand',
		bindKey: {
			win: 'Ctrl-Z',
			mac: 'Command-Z'
		},
		exec: function (a) {
			a.getSession().getUndoManager().undo(false)
		},
		readOnly: false
	});

	k.commands.addCommand({
		name: 'redoCommand',
		bindKey: {
			win: 'Ctrl-Shift-Z',
			mac: 'Command-Shift-Z'
		},
		exec: function (a) {
			a.getSession().getUndoManager().redo(false)
		},
		readOnly: false
	});

	httpGet(f);
	k.loadUrl = function (a) {
		f = a;
		g = getLangFromFilename(f);
		i = "text/" + g;
		if (g !== "plain") k.getSession().setMode("ace/mode/" + g);
		httpGet(f)
	};
	
	return k
}

function onBodyLoad() {
	var c = {};
	var d = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, a, b) {
		c[a] = b
	});
	var e = createEditor("editor", c.file, c.lang, c.theme);
	var f = createTree("tree", e);
	createFileUploader("uploader", f, e)
};