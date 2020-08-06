package Day8;

public class BitWise {
    public static void main(String[] args) {
        int i = 10;
        int j = 6;
        String iBinaryString = Integer.toBinaryString(i);
        String jBinaryString = Integer.toBinaryString(j);
        String iOrJBinaryString = Integer.toBinaryString(i | j);
        String iAndJBinaryString = Integer.toBinaryString(i & j);

        //1010
        //0110
        System.out.println("| operator");
        System.out.println(iBinaryString);
        System.out.println(jBinaryString);
        System.out.println(iOrJBinaryString);
        System.out.println("");
        System.out.println("& operator");
        System.out.println(iBinaryString);
        System.out.println(jBinaryString);
        System.out.println(iAndJBinaryString);

        //  (expressionA && expressionB)




    }
}
