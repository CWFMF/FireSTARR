NAME=$1
FILE=`find -type f -name wx.csv | grep firestarr | head -n 1`
COLUMN=$(head -1 ${FILE} | tr -s ',' '\n' | nl -nln |  grep "$NAME" | cut -f1)
find -type f -name wx.csv | grep firestarr | xargs -I {} sed 1d {} | awk -F, "{print \$${COLUMN}}"
