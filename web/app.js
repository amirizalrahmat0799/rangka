// The page around rangka.wasm: the generator itself is the C++ core, compiled to WebAssembly.

const $ = (id) => document.getElementById(id);

// ---- the WebAssembly module ----------------------------------------------------------------------------

/** Loads rangka.wasm with the three WASI functions it imports (only used if something prints to stderr). */
async function loadCore() {
  let memory;
  const wasi = {
    fd_write(fd, iovs, count, written) {
      const view = new DataView(memory.buffer);
      let text = "";
      let total = 0;
      for (let i = 0; i < count; i++) {
        const ptr = view.getUint32(iovs + i * 8, true);
        const len = view.getUint32(iovs + i * 8 + 4, true);
        text += new TextDecoder().decode(new Uint8Array(memory.buffer, ptr, len));
        total += len;
      }
      view.setUint32(written, total, true);
      console.warn(text);
      return 0;
    },
    fd_seek: () => 70, // ESPIPE
    fd_close: () => 0,
  };
  const { instance } = await WebAssembly.instantiateStreaming(fetch("rangka.wasm"), {
    wasi_snapshot_preview1: wasi,
  });
  memory = instance.exports.memory;
  instance.exports._initialize();
  const core = instance.exports;

  // reads a [uint32 length][bytes] buffer returned by the core, then frees it
  function take(ptr) {
    const length = new DataView(memory.buffer).getUint32(ptr, true);
    const bytes = new Uint8Array(memory.buffer, ptr + 4, length).slice();
    core.rangka_free(ptr);
    return bytes;
  }

  // passes a string into wasm memory and calls fn(ptr, length)
  function withString(text, fn) {
    const bytes = new TextEncoder().encode(text);
    const ptr = core.rangka_alloc(bytes.length);
    new Uint8Array(memory.buffer, ptr, bytes.length).set(bytes);
    try {
      return fn(ptr, bytes.length);
    } finally {
      core.rangka_free(ptr);
    }
  }

  const decode = (bytes) => JSON.parse(new TextDecoder().decode(bytes));
  return {
    catalog: () => decode(take(core.rangka_catalog())),
    generate: (options) => decode(withString(options, (p, n) => take(core.rangka_generate(p, n)))),
    zip(options) {
      const bytes = withString(options, (p, n) => take(core.rangka_zip(p, n, Date.now() / 1000)));
      if (bytes[0] === 0x7b) throw new Error(decode(bytes).error); // '{': an error, not a zip
      return bytes;
    },
  };
}

// ---- the form ------------------------------------------------------------------------------------------

function cards(container, name, choices, selected) {
  container.innerHTML = "";
  for (const c of choices) {
    const label = document.createElement("label");
    label.innerHTML = `<input type="radio" name="${name}" value="${c.id}" /><span class="card"><strong></strong><small></small></span>`;
    label.querySelector("strong").textContent = c.language && c.language !== c.label ? `${c.label}` : c.label;
    label.querySelector("small").textContent = c.id === "none" ? c.description : c.language || c.description;
    label.querySelector("input").checked = c.id === selected;
    label.title = c.description;
    container.append(label);
  }
}

const value = (name) => document.querySelector(`input[name="${name}"]:checked`)?.value ?? "none";

function readOptions() {
  const kind = value("kind");
  return {
    name: $("name").value,
    frontend: kind === "backend" ? "none" : value("frontend"),
    backend: kind === "frontend" ? "none" : value("backend"),
    database: kind === "frontend" ? "none" : value("database"),
    package: $("package").value.trim(),
    ci: $("ci").checked,
  };
}

const toText = (o) =>
  Object.entries(o)
    .filter(([k, v]) => !(k === "package" && (!v || o.backend !== "spring")))
    .map(([k, v]) => `${k}=${v}`)
    .join("\n");

function cliCommand(o, projectName) {
  const parts = ["rangka new", projectName || o.name];
  if (o.frontend !== "none") parts.push(`--frontend ${o.frontend}`);
  if (o.backend !== "none") parts.push(`--backend ${o.backend}`);
  if (o.database !== "none") parts.push(`--db ${o.database}`);
  if (o.backend === "spring" && o.package) parts.push(`--package ${o.package}`);
  if (!o.ci) parts.push("--no-ci");
  return parts.join(" ");
}

// ---- the file explorer ---------------------------------------------------------------------------------

let selectedPath = "README.md";
const closedFolders = new Set();

function buildTree(files) {
  const root = { folders: new Map(), files: [] };
  for (const f of files) {
    const parts = f.path.split("/");
    let node = root;
    for (const part of parts.slice(0, -1)) {
      if (!node.folders.has(part)) node.folders.set(part, { folders: new Map(), files: [] });
      node = node.folders.get(part);
    }
    node.files.push({ name: parts.at(-1), file: f });
  }
  return root;
}

function renderTree(node, prefix, onSelect) {
  const ul = document.createElement("ul");
  const folders = [...node.folders.entries()].sort(([a], [b]) => a.localeCompare(b));
  for (const [name, child] of folders) {
    const path = prefix + name + "/";
    const li = document.createElement("li");
    li.className = "folder" + (closedFolders.has(path) ? " closed" : "");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = name;
    button.onclick = () => {
      li.classList.toggle("closed");
      if (li.classList.contains("closed")) closedFolders.add(path);
      else closedFolders.delete(path);
    };
    li.append(button, renderTree(child, path, onSelect));
    ul.append(li);
  }
  for (const { name, file } of node.files.sort((a, b) => a.name.localeCompare(b.name))) {
    const li = document.createElement("li");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = name;
    button.title = file.path;
    if (file.path === selectedPath) button.classList.add("selected");
    button.onclick = () => onSelect(file);
    li.append(button);
    ul.append(li);
  }
  return ul;
}

// ---- wiring --------------------------------------------------------------------------------------------

const core = await loadCore();
const catalog = core.catalog();
$("version").textContent = catalog.version;
const without = (list) => list.filter((c) => c.id !== "none");
cards($("frontends"), "frontend", without(catalog.frontends), "react");
cards($("backends"), "backend", without(catalog.backends), "spring");
cards($("databases"), "database", catalog.databases, "postgres");

let current = null;

function showFile(file) {
  selectedPath = file.path;
  $("file-name").textContent = file.path;
  $("file-content").textContent = file.content;
  $("file-content").classList.toggle("wrap", file.path.endsWith(".md"));
  $("file-content").scrollTop = 0;
  for (const b of document.querySelectorAll(".tree button.selected")) b.classList.remove("selected");
  for (const b of document.querySelectorAll(".tree li:not(.folder) > button")) {
    if (b.title === file.path) b.classList.add("selected");
  }
}

function update() {
  const options = readOptions();
  const kind = value("kind");
  $("frontend-field").hidden = kind === "backend";
  $("backend-field").hidden = kind === "frontend";
  $("database-field").hidden = kind === "frontend";
  $("package-field").hidden = options.backend !== "spring";

  const result = core.generate(toText(options));
  $("error").hidden = result.ok;
  $("download").disabled = !result.ok;
  if (!result.ok) {
    $("error").textContent = result.error;
    $("command").textContent = cliCommand(options);
    return;
  }
  current = { options, result };
  $("command").textContent = cliCommand(options, result.name);
  $("project-title").textContent = result.name;
  $("file-count").textContent = `${result.files.length} files`;
  $("next-steps").textContent = result.nextSteps.join("\n");
  if (!$("package").value || $("package").dataset.auto === "1") {
    $("package").placeholder = `com.example.${result.name.replaceAll("-", "")}`;
  }

  const tree = $("tree");
  tree.innerHTML = "";
  tree.append(renderTree(buildTree(result.files), "", showFile));
  showFile(result.files.find((f) => f.path === selectedPath) ?? result.files.find((f) => f.path === "README.md"));
}

$("form").addEventListener("input", update);
$("form").addEventListener("change", update);
$("form").addEventListener("submit", (event) => {
  event.preventDefault();
  if (!current) return;
  try {
    const zip = core.zip(toText(current.options));
    const url = URL.createObjectURL(new Blob([zip], { type: "application/zip" }));
    const a = Object.assign(document.createElement("a"), { href: url, download: `${current.result.name}.zip` });
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  } catch (e) {
    $("error").hidden = false;
    $("error").textContent = e.message;
  }
});
$("copy").addEventListener("click", async () => {
  await navigator.clipboard.writeText($("command").textContent);
  $("copy").textContent = "Copied";
  setTimeout(() => ($("copy").textContent = "Copy"), 1200);
});

update();
