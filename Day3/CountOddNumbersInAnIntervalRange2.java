package Day3;

public class CountOddNumbersInAnIntervalRange2 {
    public static void main(String[] args) {
        //整除
        int m = 5 % 2; // m: 1

        //1. 定义input
        int low = 2;
        int high = 8;
        //2. 定义output
        int output = countOdds(low, high);
        //3. 调用目标函数，将结果存到output
        System.out.println(output);
            //调用sout,把output输出在console上
    }
    public static int countOdds(int low, int high) {
        int res = 0;
        if (low % 2 == 1 || high % 2 == 1){
            res = res + (high - low) / 2 + 1;
        } else {
            res = res + (high - low) / 2;
        }
        return res;
    }
}

