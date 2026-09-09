cd /Users/z0me/Documents/Arduino/cell_explorer
echo "Name this commit:"
read name
git add .
git commit -m "$name"
git push origin master