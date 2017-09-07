# leon

The **leon** set of utilities are a directory-major filesystem cleanup utility.  Originally developed for UD's Mills cluster to address problems with recursive removal of large Lustre directories, a central feature of **leon** is rate-limiting of calls to file-centric functions like `stat()` and `unlink()`.  This led to the **lrm** and **ldu** utilities, short for **leon rm** and **leon du**, in reference to the basic OS utilities they mimic.  Indeed, command line options to **lrm** and **ldu** parallel those of their counterparts.

The **leon** program itself was later written to make use of the same rate-limited library of functions.  When Lustre usage approached 100%, it became necessary to institute a cleanup procedure.  Ideally, the cleanup program would descend a directory tree looking for any filesystem entity within for which the modification timestamp was less than some threshold.  If found, the entire directory should remain untouched.  This algorithm was in response to users' having large static datasets present on the Lustre filesystem, which would not be modified but would be in-use with other collocated files.

The program name is a reference to the "cleaner" named Leon in the movie, "The Professional."

