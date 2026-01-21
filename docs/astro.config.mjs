// @ts-check
import { defineConfig } from "astro/config";

// Official integrations

import react from "@astrojs/react";
import starlight from "@astrojs/starlight";
import tailwindcss from "@tailwindcss/vite";

// Unofficial integrations

import icon from "astro-icon";
import starlightKbd from "starlight-kbd";
import starlightScrollToTop from "starlight-scroll-to-top";

import sitemap from "@astrojs/sitemap";

const websiteUrl = "https://williamkaroldicioccio.github.io/saturn/";

// https://astro.build/config
export default defineConfig({
  base: "/saturn/",
  site: websiteUrl,
  integrations: [
    starlight({
      title: "Saturn Docs",
      description: "Documentation for the Saturn game engine",
      components: {
        Hero: "./src/components/CustomHero.astro",
        SocialIcons: "./src/components/CustomSocialIcons.astro",
      },
      customCss: ["./src/styles/global.css"],
      editLink: {
        baseUrl: "https://github.com/WilliamKarolDiCioccio/saturn/pulls",
      },
      lastUpdated: true,
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/WilliamKarolDiCioccio/saturn",
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
              label: "Project Scaffolding",
              translations: {
                en: "Project Scaffolding",
                it: "Struttura del progetto",
              },
              slug: "collaborator/project_scaffolding",
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
        starlightKbd({
          types: [
            { id: "windows", label: "Windows", default: true },
            { id: "linux", label: "Linux" },
            { id: "mac", label: "macOS" },
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
  vite: { plugins: [tailwindcss()] },
});
