#!/usr/bin/env bash

HDFS_HOME=$(condor_config_val HDFS_HOME)

if [ HDFS_HOME="Not Defined: HDFS_HOME" ] ; then
	HDFS_HOME=$(condor_config_val LIBEXEC)/hdfs
fi
#echo $HDFS_HOME

CLASSPATH=$HDFS_HOME

for f in $HDFS_HOME/hadoop*.jar; do
	CLASSPATH=${CLASSPATH}:$f;
done

#"COMMAND" = "dfs" 
CLASS=org.apache.hadoop.fs.FsShell

# add libs to CLASSPATH
for f in $HDFS_HOME/lib/*.jar; do
	CLASSPATH=${CLASSPATH}:$f;
done

#if $cygwin; then
#  CLASSPATH=`cygpath -p -w "$CLASSPATH"`
#fi

#export CLASSPATH=$CLASSPATH

java "-classpath" $CLASSPATH $CLASS "-put" "$@"

