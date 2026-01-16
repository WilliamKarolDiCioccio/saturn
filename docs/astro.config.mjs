// @ts-check
import { defineConfig } from "astro/config";

// Official integrations

import starlight from "@astrojs/starlight";
import react from "@astrojs/react";

// Unofficial integrations

import icon from "astro-icon";
import starlightThemeBlack from "starlight-theme-black";
import starlightKbd from "starlight-kbd";
import starlightScrollToTop from "starlight-scroll-to-top";

import sitemap from "@astrojs/sitemap";

const websiteUrl = "https://williamkaroldicioccio.github.io/mosaic/";

// https://astro.build/config
export default defineConfig({
  base: "/mosaic/",
  site: websiteUrl,
  integrations: [
    starlight({
      title: "Mosaic Docs",
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/WilliamKarolDiCioccio/mosaic",
        },
      ],
      sidebar: [
        {
          label: "Introduction",
          translations: {
            en: "Introduction",
            it: "Introduzione",
          },
          items: [
            {
              label: "Overview",
              translations: { en: "Overview", it: "Panoramica" },
              slug: "intro/overview",
            },
            {
              label: "License",
              translations: { en: "License", it: "Licenza" },
              slug: "intro/license",
            },
          ],
        },
        {
          label: "User guide",
          translations: {
            en: "User guide",
            it: "Guida utente",
          },
          items: [
            {
              label: "Getting started",
              translations: { en: "Getting started", it: "Per iniziare" },
              slug: "user/getting_started",
              badge: "WIP",
            },
          ],
        },
        {
          label: "Collaborator guide",
          translations: {
            en: "Collaborator guide",
            it: "Guida collaboratori",
          },
          items: [
            {
              label: "Environment setup",
              translations: {
                en: "Environment setup",
                it: "Configurazione ambiente",
              },
              slug: "collaborator/environment_setup",
            },
            {
              label: "Build Instructions",
              translations: {
                en: "Build Instructions",
                it: "Istruzioni di compilazione",
              },
              slug: "collaborator/build_instructions",
            },
            {
              label: "C++ conventions",
              translations: { en: "C++ conventions", it: "Convenzioni C++" },
              slug: "collaborator/cpp_conventions",
            },
            {
              label: "GitHub repository conventions",
              translations: {
                en: "GitHub repository conventions",
                it: "Convenzioni repository GitHub",
              },
              slug: "collaborator/gh_repo_conventions",
            },
          ],
        },
        {
          label: "Engineering",
          translations: {
            en: "Engineering",
            it: "Ingegneria",
          },
          autogenerate: { directory: "engineering" },
        },
        {
          label: "API reference",
          translations: {
            en: "API reference",
            it: "Riferimento API",
          },
          autogenerate: { directory: "reference" },
        },
      ],
      defaultLocale: "root",
      locales: {
        root: {
          label: "English",
          lang: "en",
        },
        it: {
          label: "Italiano",
          lang: "it",
        },
      },
      plugins: [
        starlightThemeBlack({
          navLinks: [
            {
              label: "Docs",
              link: "/intro/overview",
            },
          ],
          footerText:
            "Built & designed by [shadcn](https://twitter.com/shadcn). Ported to Astro Starlight by [Adrián UB](https://github.com/adrian-ub). The source code is available on [GitHub](https://github.com/adrian-ub/starlight-theme-black).",
        }),
        starlightKbd({
          types: [
            { id: "mac", label: "macOS" },
            { id: "windows", label: "Windows", default: true },
          ],
          globalPicker: false,
        }),
        starlightScrollToTop({
          position: "left",
          tooltipText: "Back to top",
          showTooltip: true,
          smoothScroll: true,
          threshold: 20,
        }),
      ],
    }),
    icon(),
    react(),
    sitemap(),
  ],
});
