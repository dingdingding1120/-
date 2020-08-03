package Day6;

public class NumberOfGoodPairs {
    public static void main(String[] args) {
        int[] nums = new int[]{1, 2, 3, 1, 1, 3};
        int output = numIdenticalPairs(nums);
        System.out.println(output);
    }
    public static int numIdenticalPairs(int[] nums){
        int count = 0;
        for (int i = 0; i < nums.length; i++){
            int num1 = nums[i];
            for (int j = i + 1; j < nums.length; j++){
                 int num2 = nums[j];
                 if(num1 == num2){
                    count = count + 1;
                 }
            }
        }
        return count;
    }
}
