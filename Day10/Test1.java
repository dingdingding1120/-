package Day10;

public class Test1 {
    public static void main(String[] args) {
        String str = "www-runoob-com";
        String[] temp;
        String delimeter = "-";  // 指定分割字符
        temp = str.split(delimeter); // 分割字符串
        // 普通 for 循环
        String day = temp[0];
        String month = temp[1];
        String year = temp[2];
        System.out.println(day);

    }
}
