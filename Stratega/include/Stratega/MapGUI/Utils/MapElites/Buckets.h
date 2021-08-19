//#pragma once
//#include <vector>
//#include <Stratega/MapGUI/MapState.h>
//
///**
// * This class manages the dimension of the feature and how its values are organised in buckets
// * taking into consideration the minValue, maxValue and bucketSize.
// *
// * There are different solutions for this problem and some design questions that affect these:
// * 1) do the minValue are maxValue important enough to be contained in their own independent cell?
// * 2) What happens if the value is lower/higher than minValue/maxValue?
// * 3) What happens with decimal values?
// *
// * The decisions taken are the following:
// * 1) Yes they are
// * 2) The first and last buckets will contain <=minValue and >=maxValue, respectively
// * 3) They will be rounded before the calculations so 0.3 --> 0; 0.5 --> 1; 99.8 --> 100
// *
// * The buckets id are calculated for the interval excluding minValue and maxValue so for [minValue + 1, maxValue - 1].
// * This id calculation returns a value between 1 and nBuckets, and it is assume the buckets contain bucketSize numbers.
// * minValue will be assigned id 0 and maxValue will be assigned nBuckets + 1
// * The total number of buckets for the map will be nBuckets + 2
// *
// * Example 1:
// * minValue = 0; maxValue = 100; bucketSize = 10
// *
// * id:0 --> [<=0]
// * id:1 --> [1 - 10]
// * id:2 --> [11 - 20]
// * id:3 --> [21 - 30]
// * id:4 --> [31 - 40]
// * id:5 --> [41 - 50]
// * id:6 --> [51 - 60]
// * id:7 --> [61 - 70]
// * id:8 --> [71 - 80]
// * id:9 --> [81 - 90]
// * id:10 --> [91 - 99]
// * id:11 --> [>=100]
// *
// * Example 2:
// * minValue = 5; maxValue = 33; bucketSize = 10
// * id:0 --> [<=5]
// * id:1 --> [6 - 15]
// * id:2 --> [16 - 25]
// * id:3 --> [26 - 32]
// * id:4 --> [>=33]
// *
// * It is possible that that the one-to-last bucket will not contain bucketSize numbers but this is
// * expected and preferable to other options.
// */
//
//namespace SGA
//{
//    class Buckets{
//
//    public:
//        static int getMapIdx(double value, int minValue, int maxValue, int bucketSize);
//        static int getMapNBuckets(int minValue, int maxValue, int bucketSize);
//
//        /**
//        * Generate and array with the information of the range of values contained in each bucket
//        * @param minValue
//        * @param maxValue
//        * @param bucketSize
//        * @return
//        */
//        static std::vector<std::string> getMapRangesInfo(int minValue, int maxValue, int bucketSize);
//
//    private:
//        /**
//        * This method returns the id (bucket) that corresponds to the value.
//        * The id starts in 1
//        * @param value
//        * @param minValue
//        * @param bucketSize
//        * @return
//        */
//        static int getIntervalBucketId(double value, int minValue, int bucketSize);
//    };
//}