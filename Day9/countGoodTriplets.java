package Day9;

public class countGoodTriplets {
    public static void main(String[] args) {
        int[] arr = new int[]{1, 1, 2, 2, 3};
        int a = 0;
        int b = 0;
        int c = 1;
        int output = countGoodTriplets(arr, a, b, c);
        System.out.println(output);
    }
    public static int countGoodTriplets(int[]arr, int a, int b, int c){
        int count = 0;
        for(int i = 0; i < arr.length; i++){
            for(int j = 1; j < arr.length; j++){
                int temp1 = Math.abs(arr[i] - arr[j]);
                for(int k = 2; k < arr.length; k++){
                    int temp2 = Math.abs(arr[j] - arr[k]);
                    int temp3 = Math.abs(arr[i] - arr[k]);
                    if(i < j && j < k && temp1 <= a && temp2 <= b && temp3 <= c){
                        count += 1;
                    }
                }
            }
        }
        return count;
    }
}

