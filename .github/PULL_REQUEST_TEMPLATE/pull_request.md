## 🔀 Pull Request – Saturn Game Engine

### Pre-PR Checklist ✅

Before opening this pull request, please confirm:

- [ ] You've checked for related [issues](../../issues) or pull requests 🔍
- [ ] Your branch is up to date with `main` and has no merge conflicts 🔄
- [ ] All tests pass on your local machine (unit + integration) 🧪
- [ ] You've documented new functionality or updated existing docs where needed 📚
- [ ] You’ve tested the change across relevant platforms (Windows, Linux, Web, Android) if applicable 🌍

---

### 📋 Summary

_A clear and concise explanation of what this PR adds, changes, or fixes._

**Example:** Implements input rebinding support with persistent configuration, allowing users to remap keys at runtime.

---

### 🔗 Related Issues

_Reference any issues this PR addresses._

**Example:**  
Closes #42  
Related to #37

---

### 💻 Implementation Details

_Describe the technical approach, affected subsystems, and any architectural decisions._

**Example:**

- Added `InputBindingSet` to manage rebinding logic.
- Refactored `InputContext` to support rebinding at runtime.
- Updated serialization to persist custom bindings to disk.

---

### 🧪 Steps to Test

_How reviewers can verify the changes. Be specific._

**Example:**

1. Launch the engine.
2. Open the input settings panel (F3).
3. Remap the "Jump" action to a different key.
4. Restart the engine — ensure the binding persists.
5. Confirm the action fires correctly in both debug and release builds.

---

### 📸 Screenshots or Logs (Optional)

_Add images, logs, or visual diffs if they help explain the PR._

---

### 📂 Additional Notes

_Include any known limitations, future work, or areas of concern._

**Example:**  
This does not currently support controller input rebinding — planned for a separate PR.

---

Thanks for contributing to Saturn! 💚
