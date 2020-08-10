package Aug10;

import java.util.Arrays;

public class CanMakeArithmeticProgression {
    public static void main(String[] args) {
        int[] arr = new int[]{20,17,-18,-13,13,-14,-8,10,1,14,-19};
        boolean ans = canMakeArithmeticProgression(arr);
        System.out.println(ans);
    }
    public static boolean canMakeArithmeticProgression(int[] arr) {
        boolean ans = true;
        Arrays.sort(arr);
        for (int k = 1; k < arr.length - 1; k++) {
            if (arr[k] - arr[k - 1] != arr[k + 1] - arr[k]) {
                ans = false;
            }
        }
        return ans;
    }
}
