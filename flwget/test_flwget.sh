PROTOCOL="ftp"
MIRROR="distro.ibiblio.org/pub/linux/distributions/tinycorelinux/"
#REPOSITORY="2.x/release"
#FILE="tinycore_2.2rc2.iso"
REPOSITORY="2.x/tce"
FILE="abiword.tce"
TARGET="/tmp"

#echo "trying: aterm +tr +sb -bg white -fg black -geometry 80x4 -e busybox wget -c " $PROTOCOL://$MIRROR/$REPOSITORY/$FILE "2>/dev/null"
aterm +tr +sb -bg white -fg black -geometry 80x4 -e busybox wget -c "$PROTOCOL:"//"$MIRROR"/"$REPOSITORY"/"$FILE" 2>/dev/null
echo "Result: $?"
rm "$FILE"

./flwget "$PROTOCOL"://"$MIRROR"/"$REPOSITORY"/"$FILE" "$TARGET"
echo "Result: $?"
rm "$TARGET"/"$FILE"
