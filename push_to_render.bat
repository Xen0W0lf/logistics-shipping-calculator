@echo off
echo ==============================================
echo Pushing Updates to GitHub and Render.com...
echo ==============================================

git add .
git commit -m "Add Automatic Hub Routing and Autocomplete"
git push origin main

echo.
echo ==============================================
echo SUCCESS! Your code has been pushed.
echo Render.com will now automatically deploy the updates.
echo It usually takes 2-3 minutes.
echo ==============================================
pause
