This document describes the steps to create a new release.

We use the YYYY.0M.0D.MINOR calendar versioning (CalVer) scheme.

1. Update the release (or version) number in the following files:

   * ``android/build.gradle``
   * ``src/defs.h``

   In the case of ``android/build.gradle``, the version code needs to be
   increased.

2. Add a description about the new release to ``docs/News.md``.

3. Create a changelog with the same description in ``metadata/en-US/changelogs``.
   It should be named after the Android version code and use the extension
   ``.txt``.

4. Create a Git tag:

   ``git tag -a <release number> -m <short description>``

5. Build the Windows executables (x86 and x64) using raylib's default GLFW
   backend and bundle each of them in a ZIP file along with the ``assets``
   directory, the file ``LICENSE.txt``, and a directory named ``docs``
   containing the PDF manual and, in a subdirectory named ``licenses``, the text
   files for the GPLv3 and CC BY-SA 4.0 licenses.

6. Build and sign a release APK for Android.

7. Push to GitHub:

   ``git push --tags origin main``

8. Finally, create the release on GitHub with the same description added to
   News.md and include the ZIP files with the Windows builds, the Android APK,
   and the PDF manual as release assets.

