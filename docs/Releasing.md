This document describes the steps to create a new release.

We use the YYYY.0M.0D.MINOR calender versioning (CalVer) scheme.

1. Update the release (or version) number in the following files:

   * ``android/app/build.gradle``
   * ``src/defs.h``

2. Add a description about the new release to ``docs/News.md``.

3. Create a Git tag:

   ``git tag -a <release number> -m <short description>``

4. Build the Windows executables (x86 and x64) using raylib's default GLFW
   backend and bundle each of them in a ZIP file along with the ``assets``
   directory, the file ``LICENSE.txt``, and a directory named ``docs``
   containing the PDF manual and, in a subdirectory named ``licenses``, the
   text text files for the GPLv3 and CC BY-SA 4.0 licenses.

5. Build and sign a release APK for Android.

6. Push to GitHub:

   ``git push --tags origin main``

7. Finally, create the release on GitHub with the same description added to
   News.md and include the ZIP files with the Windows builds and the Android
   APK as release assets.

