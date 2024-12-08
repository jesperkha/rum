make installer
powershell -File scripts/zip_installer.ps1

read -p "Enter tag name: vx.x.x" tag

python scripts/download_button.py

git add .
git commit -m "$tag"
git push origin dev

notes=$(sed '/<br>/q' changelog.md | sed '/<br>/d')

gh release create $tag --draft \
    --title "rum $tag" \ 
    --notes "$notes" \
    dist/RumInstaller.zip
