import fs from "node:fs";
import path from "node:path";
import { execSync } from "node:child_process";

const INPUT_DIR = "./src/mmds";
const OUTPUT_DIR = "./src/assets/mmds";

// Ensure Input Directory exists
if (!fs.existsSync(INPUT_DIR)) {
  console.warn(
    `⚠️ Input directory "${INPUT_DIR}" not found. Skipping Mermaid sync.`
  );
  process.exit(0);
}

// Ensure Output Directory exists
if (!fs.existsSync(OUTPUT_DIR)) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

const mmdFiles = fs.readdirSync(INPUT_DIR).filter((f) => f.endsWith(".mmd"));

// Create a Set of all expected SVG filenames
const expectedSvgs = new Set();
mmdFiles.forEach((file) => {
  const base = file.replace(".mmd", "");
  expectedSvgs.add(`${base}_light.svg`);
  expectedSvgs.add(`${base}_dark.svg`);
});

// Generation Phase
mmdFiles.forEach((file) => {
  const inputPath = path.join(INPUT_DIR, file);
  const baseName = file.replace(".mmd", "");

  // Define the two variants
  const variants = [
    { suffix: "_light", theme: "default" },
    { suffix: "_dark", theme: "dark" },
  ];

  variants.forEach(({ suffix, theme }) => {
    const outputPath = path.join(OUTPUT_DIR, `${baseName}${suffix}.svg`);

    // Check if we need to regenerate (missing file or source is newer)
    const shouldGenerate =
      !fs.existsSync(outputPath) ||
      fs.statSync(inputPath).mtime > fs.statSync(outputPath).mtime;

    if (shouldGenerate) {
      console.log(`🎨 Generating [${theme}] version for: ${file}`);
      try {
        execSync(
          `pnpm exec mmdc -i "${inputPath}" -o "${outputPath}" -t ${theme} -b transparent -p puppeteer-config.json`,
          { stdio: "inherit" }
        );
      } catch (err) {
        console.error(`❌ Failed to convert ${file} (${theme}):`, err.message);
      }
    }
  });
});

// Cleanup Phase
const actualSvgs = fs.readdirSync(OUTPUT_DIR).filter((f) => f.endsWith(".svg"));

actualSvgs.forEach((svgFile) => {
  if (!expectedSvgs.has(svgFile)) {
    const stalePath = path.join(OUTPUT_DIR, svgFile);
    console.log(`🗑️ Removing orphaned SVG: ${svgFile}`);
    fs.unlinkSync(stalePath);
  }
});

console.log("✅ Mermaid sync (Dual-Theme) complete.");

process.exit(0);
