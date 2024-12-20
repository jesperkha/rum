make installer
powershell -File scripts/zip_installer.ps1

python scripts/download_button.py

read -p "Enter tag name: vx.x.x: " tag

git add .
git commit -m "$tag"
git push origin dev

notes=$(sed '/<br>/q' changelog.md | sed '/<br>/d')

gh release create "$tag" --title "rum $tag" --notes "$notes" dist/RumInstaller.zip

git switch main
git merge main dev
git push origin main