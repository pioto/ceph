#!/bin/sh -e

echo "starting hadoop-terasort test"

# bail if $TESTDIR is not set as this test will fail in that scenario
[ -z $TESTDIR ] && { echo "\$TESTDIR needs to be set, but is not. Exiting."; exit 1; }

command0="export JAVA_HOME=/usr/lib/jvm/default-java"
#command1="$TESTDIR/apache_hadoop/bin/hadoop dfs -mkdir /tests"
#command2="$TESTDIR/apache_hadoop/bin/hadoop dfs -mkdir /tests/terasort_data"
command2="$TESTDIR/apache_hadoop/bin/hadoop jar $TESTDIR/apache_hadoop/build/hadoop-*examples*jar teragen 20000000 /tests/terasort_input"
command3="$TESTDIR/apache_hadoop/bin/hadoop jar $TESTDIR/apache_hadoop/build/hadoop-*examples*jar terasort /tests/terasort_input /tests/terasort_output"
command4="$TESTDIR/apache_hadoop/bin/hadoop jar $TESTDIR/apache_hadoop/build/hadoop-*examples*jar teravalidate /tests/terasort_output /tests/teravalidate_output"
#command5="$TESTDIR/apache_hadoop/bin/hadoop dfs -rmr /tests"
#command1="mkdir -p $TESTDIR/hadoop_input"
#command2="wget http://ceph.com/qa/hadoop_input_files.tar -O $TESTDIR/hadoop_input/files.tar"
#command3="cd $TESTDIR/hadoop_input"
#command4="tar -xf $TESTDIR/hadoop_input/files.tar"
#command5="$TESTDIR/apache_hadoop/bin/hadoop fs -mkdir wordcount_input"
#command6="$TESTDIR/apache_hadoop/bin/hadoop fs -put $TESTDIR/hadoop_input/*txt wordcount_input/"
#command7="$TESTDIR/apache_hadoop/bin/hadoop jar $TESTDIR/apache_hadoop/build/hadoop-example*jar wordcount wordcount_input wordcount_output"
#command8="rm -rf $TESTDIR/hadoop_input"


echo $command0
$command0
#echo $command1
#$command1
echo $command2
$command2
echo $command3
$command3
echo $command4
$command4
#echo $command5
#$command5

echo "completed hadoop-terasort test"
exit 0
