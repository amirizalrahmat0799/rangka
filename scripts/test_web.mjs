// Checks the web page in a real browser: it loads the WebAssembly core, previews files, and downloads a zip
// that unzips into the same files as the CLI makes.
//   node scripts/test_web.mjs <url> [screenshot-dir]
import { chromium } from "playwright";
import { mkdtempSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { execFileSync } from "node:child_process";

const [url, shots] = process.argv.slice(2);
const browser = await chromium.launch();
const failures = [];
const check = (ok, what) => {
  console.log(`${ok ? "ok  " : "FAIL"} ${what}`);
  if (!ok) failures.push(what);
};

for (const scheme of ["light", "dark"]) {
  const page = await browser.newPage({ viewport: { width: 1360, height: 860 }, colorScheme: scheme, acceptDownloads: true });
  const errors = [];
  page.on("pageerror", (e) => errors.push(String(e)));
  page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
  await page.goto(url);
  await page.waitForSelector(".tree button");

  if (scheme === "light") {
    check((await page.textContent("#file-count")).includes("files"), "the default project is previewed");
    check((await page.textContent("#command")) === "rangka new my-app --frontend react --backend spring --db postgres", "CLI command for the defaults");

    await page.fill("#name", "Kedai Runcit");
    await page.click("label:has(input[value=vue])");
    await page.click("label:has(input[value=go])");
    check((await page.textContent("#project-title")) === "kedai-runcit", "the name becomes kebab-case");
    check((await page.textContent("#command")).includes("--frontend vue --backend go --db postgres"), "CLI command follows the options");
    check(await page.isHidden("#package-field"), "Java package hidden for Go");
    check(await page.isHidden("label:has(input[value=h2])"), "H2 hidden for Go");

    await page.click("label:has(input[value=spring])");
    await page.click("label:has(input[value=h2])");
    check((await page.textContent("#command")).includes("--backend spring --db h2"), "H2 offered with Spring Boot");
    await page.click("label:has(input[value=express])");
    check((await page.textContent("#command")).includes("--db postgres"), "leaving Spring Boot switches H2 back to PostgreSQL");
    await page.click("label:has(input[value=angular])");
    await page.click("label:has(input[value=mysql])");
    check((await page.textContent("#command")).includes("--frontend angular --backend express --db mysql"), "Angular and MySQL");
    check((await page.textContent("#tree")).includes("angular.json"), "Angular files in the preview");
    await page.click("label:has(input[value=vue])");
    await page.click("label:has(input[value=go])");

    await page.click("label:has(input[value=frontend])");
    check(await page.isHidden("#backend-field"), "backend options hidden for a frontend on its own");
    check(!(await page.textContent("#command")).includes("--backend"), "no backend in the command");

    await page.fill("#name", "123");
    check(await page.isVisible("#error"), "an invalid name shows an error");
    check(await page.isDisabled("#download"), "and disables the download");

    await page.fill("#name", "toko");
    await page.click("label:has(input[value=full])");
    await page.click("label:has(input[value=express])");
    await page.click(".tree button[title='backend/src/app.ts']");
    check((await page.textContent("#file-content")).includes("createApp"), "clicking a file shows it");

    const [download] = await Promise.all([page.waitForEvent("download"), page.click("#download")]);
    const dir = mkdtempSync(join(tmpdir(), "rangka-web-"));
    const zip = join(dir, download.suggestedFilename());
    await download.saveAs(zip);
    check(download.suggestedFilename() === "toko.zip", "the zip is named after the project");
    execFileSync("unzip", ["-q", zip, "-d", dir]);
    const list = execFileSync("find", [join(dir, "toko"), "-type", "f"]).toString().trim().split("\n").length;
    check(list === Number((await page.textContent("#file-count")).split(" ")[0]), `the zip holds every previewed file (${list})`);
    console.log(`zip: ${zip}`);
  }

  if (shots) {
    await page.click("label:has(input[value=full])");
    await page.fill("#name", "kedai-runcit");
    await page.click("label:has(input[value=react])");
    await page.click("label:has(input[value=spring])");
    await page.click("label:has(input[value=postgres])");
    await page.click(".tree button[title='README.md']");
    await page.screenshot({ path: join(shots, `web-${scheme}.png`) });
  }
  check(errors.length === 0, `no console errors (${scheme})${errors.length ? ": " + errors.join("; ") : ""}`);
  await page.close();
}

await browser.close();
if (failures.length) {
  console.log(`${failures.length} failed`);
  process.exit(1);
}
