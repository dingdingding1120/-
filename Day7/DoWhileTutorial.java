package Day7;

public class DoWhileTutorial {
    public static void main(String[] args) {
        int i = 300;
        int j = 300;
        while (i < 5) {
            System.out.println("loop1");
            i++;
        }

        do {
            System.out.println("loop2");
            j++;
        } while (j < 5);
    }
}
