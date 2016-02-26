a = 258
print "initial number"
print a
upper = a >> 8
print "upper byte"
print upper
lower = a & 0xFF
print "lower byte"
print lower
newUpper = upper << 8
print "shift upper back"
print newUpper
new = newUpper + lower
print "add lower to upper"
print new