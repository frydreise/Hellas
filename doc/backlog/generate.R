library("RColorBrewer")

circleSize = 1.5

svg("backlog.svg", width=12, height=8)
#png("backlog.png", width=600, height=500, type="cairo", pointsize=13)

par(mar=c(4.1, 4.1, 0, 11))

jitter_log <- function(vals, scaler=0.05) {
  noise <- rnorm(length(vals), mean=0, sd=vals*scaler)
  vals + noise
}

data <- read.csv("backlog.csv", sep=",")

# Categories
cats = sort(unique(data$type))
data$typeID = match(data$type, cats)
set1Cols = brewer.pal(9, "Set1")
cols = set1Cols;
cols[1] = set1Cols[2]; # Probably aesthetics; blue
cols[2] = set1Cols[1]; # Probably bug; red
cols[6] = brewer.pal(8, "Set2")[6] # Normal yellow too light
data$color = cols[data$typeID]

# ROI
vals = c(1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144)
data$roi = match(data$value, vals) - match(data$effort, vals)

data$done = !is.na(data$done)
minROI = NULL
maxROI = NULL
for (i in 1:length(data$roi)){
    if (!data$done[i]){
        if (is.null(minROI))
            minROI = data$roi[i]
        else if (data$roi[i] < minROI)
            minROI = data$roi[i]
        if (is.null(maxROI))
            maxROI = data$roi[i]
        else if (data$roi[i] > maxROI)
            maxROI = data$roi[i]
    }
}
#for (i in 1:length(data$roi)){
#    if (data$done[i]){
#        if (data$roi[i] < minROI)
#            data$roi = minROI
#        else if (data$roi[i] > maxROI)
#            data$roi = maxROI
#    }
#}
roiRange = maxROI - minROI + 1
roiPalette = colorRampPalette(brewer.pal(11, "RdYlGn"))
paletteSize = roiRange;
if (roiRange %% 2 == 0){
    paletteSize = paletteSize + 1
}
roiColsRaw = roiPalette(paletteSize)
roiCols = 1:19
for (i in (-9:minROI + 10)){
    roiCols[i] = roiColsRaw[1]
}
for (i in (maxROI:9 +10)){
    roiCols[i] = roiColsRaw[roiRange]
}
for (i in 1:roiRange){
    roiCols[minROI+10 + i - 1] = roiColsRaw[i]
}

# Sort data
data <- data[with(data, order(-roi, effort, issue)),]

x = jitter_log(data$effort)
y = jitter_log(data$value)

maxEffort = max(data$effort)
maxValue = max(data$value)
x = sapply(x, function(n) min(c(n, maxEffort)))
x = sapply(x, function(n) max(c(n, 1)))
y = sapply(y, function(n) min(c(n, maxValue)))
y = sapply(y, function(n) max(c(n, 1)))

plot(
    NULL, log="xy", xlab="Effort", ylab="Value", axes=FALSE,
    #main="Issue backlog",
    xlim=c(min(x), max(x)), ylim=c(min(y), max(y))
)
effortVals = vals[1:match(maxEffort, vals)]
valueVals = vals[1:match(maxValue, vals)]
axis(1, at=effortVals)
axis(2, at=valueVals)
#box()




logHalf <- function(a, b){
    sqrt(a*b)
}



numVals = length(vals)
topVal = vals[numVals]
secondTopVal = vals[numVals-1]
topEdge = logHalf(secondTopVal, topVal)

polygon(x=c(1,topVal,topVal,1), y=c(1,1,topVal,topVal), lty=0, col=roiCols[10])

for (e in 1:(numVals-1)){
    midE = logHalf(vals[e], vals[e+1])
    nextMidE = logHalf(vals[e+1], vals[e+2])
    if (e > 1) {
        for (v in 1:e){
            roi = v-e;
            polygon(
                x=c(midE, nextMidE, nextMidE, midE),
                y=c(vals[v], vals[v+1], vals[v+2], vals[v+1]),
                col=roiCols[10+roi],
                border=roiCols[10+roi]
            )
        }
    }
    roi = -e
    polygon(
        x=c(midE, nextMidE, nextMidE),
        y=c(vals[1], vals[1], vals[2]),
        col=roiCols[10+roi],
        border=roiCols[10+roi]
    )
}

for (v in 1:(numVals-1)){
    midV = logHalf(vals[v], vals[v+1])
    nextMidV = logHalf(vals[v+1], vals[v+2])
    if (v > 1){
        for (e in 1:v){
            roi = v-e;
            polygon(
                x=c(vals[e], vals[e+1], vals[e+2], vals[e+1]),
                y=c(midV, nextMidV, nextMidV, midV),
                col=roiCols[10+roi],
                border=roiCols[10+roi]
            )
        }
    }
    roi = v
    polygon(
        x=c(vals[1], vals[1], vals[2]),
        y=c(midV, nextMidV, nextMidV),
        col=roiCols[10+roi],
        border=roiCols[10+roi]
    )
}

# blocker lines
#for (i in 1:length(data$roi)){
#	blockedBy = data$blockedBy[i]
#	toX = x[i]
#	toY = y[i]
#    if (!is.na(blockedBy)){
#		# find blocking PBI
#		for (j in 1:length(data$roi)){
#			if (data$issue[j] == blockedBy){
#				fromX = x[j]
#				fromY = y[j]
#				arrows(
#					fromX, fromY, toX, toY,
#					angle = 20,
#				)
#				break
#			}
#		}
#	}
#}


for (i in 1:length(data$roi)){
	if (!data$done[i]) {
		blockedBy = data$blockedBy[i]
		if (!is.na(blockedBy)){
			points(x[i], y[i], pch=21, bg=data$color[i], cex=circleSize - 0.25, col="grey")
		} else {
			points(x[i], y[i], pch=21, bg=data$color[i], cex=circleSize, col="black")
		}
	}
}


#points(x, y, pch=21, bg=data$color, cex=circleSize, col="black")
#points(x, y, col=data$color, cex=circleSize, lwd=3)


add_legend <- function(...) {
  opar <- par(fig=c(0, 1, 0, 1), oma=c(0, 0, 0, 0), 
    mar=c(0, 0, 0, 0), new=TRUE)
  on.exit(par(opar))
  plot(0, 0, type='n', bty='n', xaxt='n', yaxt='n')
  legend(...)
}
add_legend(legend=cats, x="topright", fill=cols, inset=c(0.02,0.15))










dev.off()


suggestRefinementAtEV = 400
suggestRefinementAtEffort = 14

# Write js file
fieldNum <- function(name, value){
    paste("    ", name, ": ", value, ",", sep="")
}
fieldStr <- function(name, value){
    paste("    ", name, ": \"", value, "\",", sep="")
}

text = "PBIs = ["
for (i in 1:length(data$roi)){
    text = c(text, "{")
    text = c(text, fieldNum("issue", data$issue[i]))
    text = c(text, fieldStr("description", data$description[i]))
    text = c(text, fieldStr("type", data$type[i]))
    text = c(text, fieldStr("typeColor", cols[data$type[i]]))
    text = c(text, fieldNum("value", data$value[i]))
    text = c(text, fieldNum("effort", data$effort[i]))
    roi = data$roi[i]
    text = c(text, fieldNum("roi", roi))
    roiForColor = min(roi, maxROI)
    roiForColor = max(roiForColor, minROI)
    text = c(text, fieldStr("roiColor", roiCols[roiForColor+10]))
    text = c(text, fieldNum("done", if (data$done[i]) "true" else "false"))
    
    if (!is.na(data$blockedBy[i])){
        text = c(text, fieldNum("blockedBy", data$blockedBy[i]))
    }

    if (data$effort[i] * data$value[i] >= suggestRefinementAtEV &&
        data$effort[i] >= suggestRefinementAtEffort){
        text = c(text, fieldStr("refine", "true"))
    }

    text = c(text, "},")
}
text = c(text, "];")

write(text, file="backlog.js")
