library("cffdrs")
library("data.table")
library("jsonlite")
library("testthat")

# precision to ensure to consider things a match
TOLERANCE <- 0.001

DIR <- normalizePath("example")
FILE_JSON <- sprintf("%s/fire.geojson", DIR)
FILE_WX_OUTPUT <- sprintf("%s/output/wx_hourly_out.csv", DIR)
FILE_FBP_OUTPUT <- sprintf("%s/output/fbp_wx_hourly_out.csv", DIR)

fire <- fromJSON(FILE_JSON)$features$properties
# fire <- unlist(fromJSON(FILE_JSON)$features$properties)
cols <- c("lat", "lon", "ffmc_old", "dmc_old", "dc_old")
init <- as.data.table(lapply(fire[cols], as.double),
    names = cols
)


read_wx <- function(path) {
    df <- fread(path)
    # setnames(df, c("Scenario"), c("ID"))
    names(df) <- toupper(names(df))
    df[, `:=`(
        YR = year(DATE),
        MON = month(DATE),
        DAY = mday(DATE),
        HR = hour(DATE),
        MIN = minute(DATE),
        ID = .I
    )]
    df[, `:=`(
        LAT = init$lat,
        LONG = init$lon
    )]
    setorder(df, "DATE", "ID")
    return(df)
}


# # don't worry about input csv hourly calculations for now
# df_fwi <- NULL
# for (id in unique(df_wx$ID)) {
#     df_fwi <- rbind(
#         df_fwi,
#         fwi(df_wx[ID == id], init = init)
#     )
# }
# setorder(df_fwi, "ID", "DATE")
# cols_cmp <- names(df)
# df_cmp <- df_fwi[, ..cols_cmp]
# df_cmp - df

# fwi()
df_wx_out <- read_wx(FILE_WX_OUTPUT)
df_wx <- df_wx_out[, -c("FFMC", "DMC", "DC", "ISI", "BUI", "FWI")]

cols_derived <- c("ISI", "BUI", "FWI")
# compare calcuation of dependent variabls
df_fwi <- df_wx_out[, -..cols_derived]
df_fwi[, `:=`(
    ISI = cffdrs:::initial_spread_index(FFMC, WS),
    BUI = cffdrs:::buildup_index(DMC, DC)
)]
df_fwi[, FWI := cffdrs:::fire_weather_index(ISI, BUI)]

check_diff <- function(a, b) {
    cols <- intersect(names(a), names(b))
    a <- as.data.table(a)[, ..cols]
    b <- as.data.table(b)[, ..cols]
    testthat::expect_equal(a, b, tolerance = TOLERANCE)
}
check_diff(df_wx_out, df_fwi)
check_diff(df_wx_out[, ..cols_derived], df_fwi)

df_wx_out[, FWI := cffdrs:::fire_weather_index(ISI, BUI)]
check_diff(df_wx_out[, ..cols_derived], df_fwi)

# df_fbp <- fbp(df_wx_out, output = "ALL")
# TODO: FIX
# Error in slope_values[["WSV"]] : subscript out of bounds
# when no FUELTYPE defined

df_wx_out[, `:=`(
    GS = 0,
    DJ = yday(DATE),
    ASPECT = 0,
    SLOPE = 0,
    FUELTYPE = "C2"
)]

df_fbp_out <- read_wx(FILE_FBP_OUTPUT)

write_sig <- function(df, filename, sigdigs = 5) {
    fwrite(signif(df[, -c("DATE", "FD")], sigdigs), filename)
}
do_fbp <- function(df) {
    filename <- sprintf("%s/output/fbp_cffdrs.csv", DIR)
    df_fbp <- as.data.table(fbp(df, output = "ALL"))
    # # HACK: want head fire ROS
    cols_rm <- intersect(names(df_fbp), names(df))
    cols <- intersect(names(df_fbp), names(df_fbp_out))
    df_fbp <- cbind(df, df_fbp[, -..cols_rm])
    # df_fbp <- merge(df, df_fbp, by = c("ID"))
    setorder(df_fbp, "SCENARIO", "DATE")
    df_fbp <- df_fbp[, ..cols]
    write_sig(df_fbp, filename)
    write_sig(df_fbp_out[, ..cols], sprintf("%s/output/fbp_out.csv", DIR))
    return(df_fbp)
}
# df_fbp <- do_fbp(df_wx_out, output = "PRIMARY")
# df_fbp <- do_fbp(df_wx_out, output = "SECONDARY")
# df_fbp <- do_fbp(df_wx_out, output = "ALL")
df_fbp <- do_fbp(df_wx_out)

# check_diff(df_fbp_out[, -c("ID")], df_fbp)

# merge(df_fbp, df_fbp_out, by = c("ID", "DATE"))

# df_fbp_out[df_fbp$FD != df_fbp_out$FD]

# a <- df_fbp_out[, -c("ID", "DATE")]
# b <- df_fbp
# cols <- intersect(names(a), names(df_fbp))
# a <- as.data.table(a)[, ..cols]
# b <- as.data.table(b)[, ..cols]
# testthat::expect_equal(a, b, tolerance = TOLERANCE)
