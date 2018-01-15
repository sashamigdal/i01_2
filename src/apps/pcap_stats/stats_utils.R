require(data.table)
require(fdoReports)

read.stats.file <- function(date,
                            interface,
                            basedir="/storage/i01/data/stats/") {

  if (is.character(date)) {
    date <- as.Date(date)
  }

  filename <- paste0(basedir,as.character(date, "/%Y/%m/%d/"), "stats.", interface,
                     ".", as.character(date,"%Y%m%d"), ".csv")

  if (!file.exists(filename)) {
    return(NULL)
  }

  max.fields <- max(count.fields(filename,sep=","))
  as.data.table(read.csv(filename, header=FALSE, col.names=paste0("V",seq_len(max.fields)),fill=TRUE))
}

msg.from.stats <- function(dat, bd) {
  g <- dat[V1 == "MSG", ]
  if (nrow(g) ==0) {
    return (data.table())
  }

  g <- g[, list(d=bd,num=sum(.SD)), .SDcols=seq(4,ncol(g),by=3)]
  g[, pretty.d:=format(d, "%m-%d")]
  g[, pretty.d.factor:=factor(pretty.d, unique(pretty.d[order(as.integer(d))]))]
  g
}

gaps.from.stats <- function(dat, bd) {
  g <- dat[V1=="GAP2",]
  if (nrow(g) == 0) {
    return(data.table())
  }
  g <- g[,.SD, .SDcols=c(2,3,4,6,7)]

  setnames(g, c("ts", "addr", "seqnum", "num", "duration"))
  g[, ts:=as.numeric(as.character(ts))]
  g[, dt:=as.POSIXct(ts, origin="1970-01-01")]
  g[, t:=hour(dt)*3600 + minutes(dt)*60 + seconds(dt)]
  g[, d:=bd]
  g[, pretty.d:=format(d, "%m-%d")]
  g[, pretty.d.factor:=factor(pretty.d, unique(pretty.d[order(as.integer(d))]))]
  g
}

extract.from.date <- function(bd, map.func, ...) {
  dat <- read.stats.file(bd, ...)
  if (is.null(dat)) {
    return(data.table())
  }
  map.func(dat, bd)
}

extract.for.interface.dates <- function(dates, interface, map.func, ...) {
  rbindlist(lapply(dates, function(x) {
    dat <- extract.from.date(x, map.func, interface=interface,...)
    if (nrow(dat) > 0) {
      dat[, iface:=interface]
    }
    dat
  }))
}


## to get gaps over some period:
## extract.for.interface.dates(businessDays("2015-07-01", "2015-07-15"), "mdsfti", gaps.from.stats)
## to get total number of msgs:
## extract.for.interface.dates(businessDays("2015-07-01", "2015-07-15"), "mdsfti", msg.from.stats)


get.all.gaps <- function(dates, interfaces=c("mdsfti", "mdnasdaq", "mdedgeny5")) {
  dat <- rbindlist(lapply(interfaces, function(x) { extract.for.interface.dates(dates, x, gaps.from.stats) }))
  if (nrow(dat) > 0) {
    ## reorder the d factors since we just catted 3 diff dt
    dat[, pretty.d.factor:=factor(pretty.d, unique(pretty.d[order(as.integer(d))]))]
    dat[, is.rth:=t>= 34200 & t < 57600]
  }
  dat
}


##  ggplot(gaps[is.rth==TRUE,], aes(x=pretty.d.factor,y=num,group=iface,color=iface)) + geom_jitter() + scale_y_log10() + theme(axis.text.x=element_text(angle=90,hjust=1))

get.all.msgs <- function(dates, interfaces=c("mdsfti", "mdnasdaq", "mdedgeny5")) {
  dat <- rbindlist(lapply(interfaces, function(x) { extract.for.interface.dates(dates, x, msg.from.stats) }))
  if (nrow(dat) > 0) {
    dat[, pretty.d.factor:=factor(pretty.d, unique(pretty.d[order(as.integer(d))]))]
  }
  dat
}

plot.by.d <- function(dat,y,title="") {
  p <- ggplot(dat, aes_string(x="pretty.d.factor", y=y, group="iface", color="iface")) + theme(axis.text.x=element_text(angle=90,hjust=1))
  if (title != "") {
    p <- p + ggtitle(title)
  }
  p
}

make.gap.report <- function(date,
                            interfaces=c("mdsfti", "mdnasdaq", "mdedgeny5"),
                            output.dir=".",
                            title="RTH Gaps in Market Data",
                            name="I01GapReport",
                            days.prior=4,
                            email.to=NULL) {
  date <- getValidDate(date)

  start.date <- nextBusinessDay(date,-days.prior)

  date.range <- businessDays(start.date, date)

  gaps <- get.all.gaps(date.range, interfaces)

  msgs <- get.all.msgs(date.range, interfaces)

  ptex <- "No data"
  pp <- list()

  if (nrow(gaps) > 0) {
    gaps <- gaps[is.rth==TRUE,]
    pp[[length(pp)+1]] <- plot.by.d(gaps, "num", "Number of missed msgs") + geom_jitter() + scale_y_log10()
    pp[[length(pp)+1]] <- plot.by.d(gaps, "t", "Time of day") + geom_jitter()
  }

  if (nrow(msgs) > 0) {
    pp[[length(pp)+1]] <- plot.by.d(msgs, "num", "Msg count") + geom_line() + geom_point()
  }

  if (length(pp) > 0) {
    ptex <- genPlotTex(pp, width=10,height=7)
  }

  tex <- paste("\\centering",
               ptex)
  report.file <- makeReportFromTex(tex,
                                   title=title,
                                   date=date,
                                   name=name,
                                   output.dir=output.dir,
                                   overwrite=TRUE,
                                   landscape=TRUE)

  if (!is.null(email.to)) {
    sendmail(from = "eq-reports@fdopartners.com",
             to   = email.to,
             subject = paste(name, " Report as of ", format(date), sep = ""),
             msg = list("Please find report attached.",
               mime_part(report.file)))
  }
  report.file
}
