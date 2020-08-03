package Day4;

public class numWaterBottles {
    public static void main(String[] args) {
        int numBottles = 15;
        int numExchange = 4;
        int output = numWaterBottles(numBottles, numExchange);
        System.out.println(output);
    }
    public static int numWaterBottles(int numBottles, int numExchange){
        int newBottles = numBottles / numExchange;
        int surplusBottles = numBottles % numExchange;
        int temp = 0;
        int res = numBottles + newBottles;
        while (newBottles > 0){
            temp = newBottles;
            newBottles = (surplusBottles + newBottles) / numExchange;
            surplusBottles = (surplusBottles + temp) % numExchange;
            res = res + newBottles;
        }
        return res;
    }
}
