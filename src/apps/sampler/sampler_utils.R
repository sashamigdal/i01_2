require(reshape2)
require(ggplot2)
require(data.table)
require(chron)
require(fdoUtils)

readSampledData <- function(filename) {
  dat <- fread(filename)

  setnames(dat, c("mic", "inst.id", "ts", "num.bid", "bid.size", "bid", "ask", "ask.size", "num.ask"))

  dat[, dt:=as.POSIXct(ts,origin="1970-01-01")]
  dat[, t:=hour(dt)*3600 + minutes(dt)*60 + seconds(dt)]
  dat[, mid:=(bid+ask)/2]
  setkey(dat, inst.id, t, mic)
#  print("verifying data...")
#  verifyData(dat)
}

verifyData <- function(dat) {

  #stopifnot(dat[, sum(bid==ask)]==0)
  stopifnot(dat[, sum(bid > ask)]==0)
  stopifnot(dat[, sum(num.bid!=0 & bid.size==0)]==0)
  stopifnot(dat[, sum(num.ask!=0 & ask.size==0)]==0)
  stopifnot(dat[, sum(num.bid==0 & bid.size!=0)]==0)
  stopifnot(dat[, sum(num.ask==0 & ask.size!=0)]==0)
  stopifnot(dat[, sum(num.ask<0 | num.bid<0)]==0)
  stopifnot(dat[, sum(bid<0 | ask<0)]==0)
  stopifnot(dat[, sum(bid.size<0|ask.size<0)]==0)

  ## make sure each stop has only one sample per time
  stopifnot(dat[, .N, by=list(mic, inst.id, ts)][, sum(N!=1)]==0)

  dat
}

pricesData <- function(dat) {
  pdt <- data.table(pricesFilesData(dat[, unique(as.Date(dt))]))
  pdt[, mid:=(bid +ask)/2]
  setkey(pdt, inst.id, t)
  pdt
}

getMatchingPrices <- function(filename) {
  dt <- readSampledData(filename)
  dt[ask>1e10,ask:=NA]
  dt[bid==0,bid:=NA]
  dt[, mid:=(bid+ask)/2]
  pd <- pricesData(dt)

  bb <- dt[pd,nomatch=0]

  ## make the mids
  bb[, ask:=ask/1e4]
  bb[, bid:=bid/1e4]

  bb[, mid:=(bid+ask)/2]
  setnames(bb, "i.mid", "tps.mid")
  setnames(bb, "i.ask", "tps.ask")
  setnames(bb, "i.bid", "tps.bid")
  bb
}

testone <- function(bb) {
  ## melt based on mids

  mm <- melt(bb, c("inst.id", "t"), c("mid", "tps.mid"), variable.name="src", value.name="mid")

  agg <- mm[, list(mid=mean(mid,na.rm=TRUE)), by=list(t,src)]

  ggplot(agg, aes(x=t,y=mid,color=src)) + geom_line()
}

testtwo <- function(bb) {
  ## melt based on mids

  mm <- melt(bb, c("inst.id", "t"), c("bid", "ask", "tps.ask", "tps.bid"), variable.name="src", value.name="value")

  agg <- mm[, list(value=mean(value,na.rm=TRUE)), by=list(t,src)]

  ggplot(agg, aes(x=t,y=value,color=src)) + geom_line()
}

testthree <- function(bb) {
  mm <- melt(bb, c("inst.id", "t", "mic"), c("bid", "ask", "tps.ask", "tps.bid"), variable.name="src", value.name="value")

  ## create a consolidated bid/ask
  cc <- rbind(mm[src %in% c("bid", "tps.bid"), list(value=max(value,na.rm=TRUE),mic=mic[which.max(value)]), by=list(inst.id, t,src)],
              mm[src %in% c("ask", "tps.ask"), list(value=min(value,na.rm=TRUE),mic=mic[which.min(value)]), by=list(inst.id, t,src)])

  ## cast the prices back to wide form
  dd <- data.table(dcast(cc, inst.id + t ~ src, value.var = "value"))
  setkey(dd, inst.id, t)
  ## cast the quote sources to wide
  ss <- data.table(dcast(cc[src %in% c("bid","ask"),], inst.id + t ~ src, value.var = "mic"))
  setkey(cc, inst.id, t)
  ## join the two back
  dd[ss, c("bid.mic", "ask.mic"):=list(i.bid, i.ask)]
  
  ## cast back to wide form
  ## dd <- data.table(dcast(cc, ... ~ src ,value.var="value"))

  ## make mids
  dd[, mid:=(bid+ask)/2]
  dd[, tps.mid:=(tps.bid+tps.ask)/2]

  dd[, bid.diff:=bid-tps.bid]
  dd[, pct.bid.diff:=bid/tps.bid-1]
  dd[, ask.diff:=tps.ask-ask]
  dd[, pct.ask.diff:=tps.ask/ask-1]
}
