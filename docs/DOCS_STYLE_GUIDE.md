# Saturn Documentation Style Guide

## 1. Purpose & Authority

**Primary Goal:** Present technical information accessibly for varying experience levels, guiding readers to comprehension rather than confronting them with raw statements.

**Target Audience:** Future engine users and contributors across multiple documentation sections with different expertise levels.

**Contribution:** Anyone may contribute via PR, pending review by team members (future: dedicated documentation team/individual).

**Scope:** All Saturn project knowledge and conventions: Engine Core, Editor, Tooling, Tutorials, Conventions, Engineering Docs, API References.

**Authority:** Single source of truth for Saturn documentation style.

**Core Principles:**

- Clarity for easy understanding
- Thoroughness for complete understanding
- When conflict arises: split content into paragraphs or add notes for detailed technical explanations

**Compliance:** Writers (team members and open source contributors) must comply with this guide.

## 2. Audience Classification & Expectations

**Experience Range:** Beginner to expert, varies by documentation section.

**User-Facing Docs:** Readable by anyone.

**Contributor-Facing Docs:** Assume intermediate to expert knowledge of CS and Saturn tech stack (C++, Dart). Do not assume engine internals knowledge—docs must build it.

**Math-Heavy Content:** Allowed in dedicated sections; minimize elsewhere and focus on CS concepts.

**Keyword Linking:** Link keywords to dedicated pages for concise pages and thorough concept understanding.

## 3. Voice

**Person:** Second person. Direct address allowed for critical information (safety measures, code of conduct).

**"We" Usage:** Allowed when referring to engine authors.

**Passive Voice:** Not accepted except for logical system relationships (e.g., "the RHI is initialized by the graphics API").

**Tone Alignment:** Instructional and confident for non-opinionated information. Flag opinionated content with disclaimers.

## 4. Tone

**Standard:** Formal.

**Exceptions:** Conversational and motivational language, humor allowed in entry-level user-facing docs.

**Content vs. Tone:** Reflect uncertainty, tradeoffs, limitations, breaking changes in content, not tone.

## 5. Grammar & Language Rules

### General Conventions

- **Regional Standard:** US English exclusively (e.g., _initialize, customize, behavior, color_)
- **Mood:** Imperative for all instructions (e.g., "Select the file" not "The file should be selected")
- **Tense:** Present tense (exceptions: post-mortems, past design decisions)

### Sentence & Paragraph Structure

- **Sentence Length:** 15–20 words
- **Paragraph Composition:** 4–6 sentences
- **Paragraph Length:** 50–80 words total

### Mechanics & Formatting

- **Capitalization:** Sentence case for headings/subheadings. Capitalize UI elements as they appear (e.g., "Click **Save Changes**")
- **Acronyms:** Spell out on first mention with acronym in parentheses (e.g., _Application Programming Interface (API)_). Use acronym alone thereafter. No periods (_CPU_ not _C.P.U._)
- **Abbreviations:** Avoid Latin abbreviations (_e.g._, _i.e._); use "for example" or "that is." If required, follow with comma (_e.g.,_)
- **Pluralization:** Lowercase "s" without apostrophe for acronyms (_VMs_, _APIs_). Use regularized plurals (_indexes_, _schemas_)

### Clarity & Precision

- **Pronoun Ambiguity:** Never start sentences with standalone "This" or "That." Follow with clarifying noun (e.g., "This configuration ensures...")
- **Object Clarity:** If two objects mentioned, repeat noun in following instruction instead of "it"
- **Gender Neutrality:** Use singular "they" or direct "you." Avoid "he/she" or "(s)he"

## 6. Terminology & Vocabulary

**Technical Jargon:** Favored when adding meaningful nuance and detailed information. Regulate by documentation section to match audience.

**Glossary:** Future detailed glossary will be mandatory once project matures.

## 7. Document Structure

**Consistency:** Every page follows predictable structure for readability, scannability, maintainability.

**Single Question Rule:** Each document clearly answers one primary question. Split multiple concepts into multiple linked pages.

### Required Sections

#### General Documentation Pages

1. **Overview:** State what system/feature/concept is. Explain why it exists and when to use it.
2. **Conceptual Explanation:** Describe core ideas and mental model. Introduce terminology gradually, link to dedicated pages.
3. **Practical Usage:** Show real-world usage with minimal but representative examples.
4. **Behavioral Details:** Describe guarantees, constraints, lifecycle. Highlight performance characteristics, side effects when relevant.
5. **Edge Cases & Limitations:** Explicitly document undefined behavior, caveats, non-obvious interactions.
6. **Related Topics:** Link to relevant systems, APIs, follow-up material.

#### Tutorials

- Linear and goal-oriented
- Each step states _why_ it exists, not just _what_ to do
- End with "What you've learned" or "Next steps"

#### API References

- Autogenerated by Codex-based docs pipeline

### Heading Rules

- Reflect conceptual hierarchy, not visual preference
- Don't skip heading levels
- Avoid headings introducing trivial content

## 8. Markdown Rules

**Philosophy:** Markdown as semantic tool, not styling language.

### Headings

- ATX-style (`#`, `##`, `###`)
- Don't exceed level 4 (`####`) unless strictly necessary
- Describe content, not act as labels (avoid "Notes", "Misc", "Other")

### Emphasis

- **Inline code:** Identifiers, symbols, file names, keywords
- **Bold:** Define terms on first introduction or highlight critical warnings only
- **Italics:** Avoid for emphasis; reserve for titles or foreign words

### Lists

- Bullet lists: unordered concepts
- Numbered lists: ordered procedures only
- Avoid deep nesting; refactor into sections if exceeding two levels

### Code Blocks

- All fenced code blocks must specify language
- Precede with sentence explaining _why code exists_
- Avoid inline explanations unless necessary for clarity

### Links

- Prefer relative links to other documentation pages
- Link text must be descriptive and meaningful
- Avoid repeated links to same target within single page

## 9. Code Examples

**Standards:** Reflect real-world usage.

### General Rules

- Examples must compile or be explicitly labeled as pseudocode
- Follow Saturn coding conventions
- Avoid placeholders unless purpose clearly explained
- Favor clarity and correctness over brevity

### Scope & Context

- Provide minimum surrounding context for understanding
- Don't assume implicit setup unless previously introduced in same document
- Explain intentional code omissions

### Comments

- Explain _why_ decisions made, not _what_ code does
- Avoid redundant comments restating obvious operations

### Error Handling

- Show error detection/propagation when relevant
- If error handling omitted, state explicitly

### Audience Adaptation

- Beginner-facing: verbose examples and explanations
- Advanced: partial snippets allowed if context clear

## 10. MDX & Starlight Components

**Usage:** Use all Starlight and custom components when they improve navigation or information identification.

**Available Components:**

- [Cards](https://starlight.astro.build/components/cards/)
- [Link Cards](https://starlight.astro.build/components/link-cards/)
- [Card Grids](https://starlight.astro.build/components/card-grids/)
- [Asides](https://starlight.astro.build/components/asides/)
- [Badges](https://starlight.astro.build/components/badges/)
- [Code](https://starlight.astro.build/components/code/)
- [File Tree](https://starlight.astro.build/components/file-tree/)
- [Icons](https://starlight.astro.build/components/icons/)
- [Link Buttons](https://starlight.astro.build/components/link-buttons/)
- [Steps](https://starlight.astro.build/components/steps/)
- [Tabs](https://starlight.astro.build/components/tabs/)

## 11. Diagrams & Visuals

**Format:** Mermaid diagrams only. All types allowed.

**Pipeline:** Custom pre-render and theme-responsive pipeline.

**Process:**

1. Write diagram code in dedicated `.mmd` file in `src/mmds` folder
2. Include in MDX using custom MermaidDiagram component with filename (no extension):
   ```tsx
   <MermaidDiagram name="git_feature_branch_flow" alt="This graph shows..." />
   ```
3. SVGs generated and themed automatically by pipeline during `dev` and `build`
4. Use `sync:mermaid` script for explicit generation

## 12. Cross-Referencing & Links

**Purpose:** Keep documentation concise while enabling deep understanding.

### Internal Links

- Link to concept definitions on first mention
- Prefer linking nouns and technical terms over verbs or generic phrases
- Avoid over-linking; guide exploration, don't distract

### Source Code References

- Link to relevant source files/symbols when documenting code-derived behavior
- Source references are supplementary, not replacement for explanations

### External Resources

- Use sparingly, only for authoritative context
- Must not be required to understand Saturn-specific behavior

### Link Maintenance

- Avoid brittle links
- Update incoming links when pages moved/renamed

## 13. Error Handling & Warnings

**Principle:** Explicit and transparent about failure modes and unsafe behavior.

### Warnings & Notes

**Warnings for:**

- Undefined behavior
- Memory safety concerns
- Platform-specific pitfalls
- Misuse leading to subtle bugs

**Notes for:**

- Non-obvious design rationale
- Performance considerations
- Optional optimizations

### Error Conditions

**Document clearly:**

- When operations can fail
- How failure is reported
- What guarantees are lost after failure
- Quote error messages verbatim when relevant

### Invalid Usage

- Explicitly state invalid usage
- Don't rely on "undefined behavior" catch-all without explanation
- If behavior intentionally unspecified, state clearly

## 14. Maintenance & Evolution

**Contribution:** Everyone may submit PRs to improve/extend docs and engine.

**Approval:** Currently handled by founder/sole maintainer. Future: dedicated individual/team approval required for merge.

**Versioning:** Deprecation/update/versioning strategy undefined (early project stage, addressing challenges as encountered).

## 15. Non-Goals & Explicit Exclusions

**Excluded content:**

- Blog posts
- Marketing material
- RFCs
