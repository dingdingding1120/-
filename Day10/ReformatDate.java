package Day10;

public class ReformatDate {
    public static void main(String[] args) {
        String date = "20th Oct 2052";
        String output = reformatDate(date);
        System.out.println(output);
    }
    public static String reformatDate(String date){
        String[] temp;
        String outputDay;
        String delimeter = " ";
        temp = date.split(delimeter);
        String day = temp[0];
        String month = temp[1];
        String year = temp[2];
        if(day.length() == 3){
            outputDay = "0" + day.charAt(0);
        } else {
            outputDay = day.substring(0, 2);
        }
        String ans = year + "-" + getMonthString(month) + "-" + outputDay;
        return ans;
    }

    public static String getMonthString(String month) {
        switch (month)
        {
            case "Jan":
                return "01";
            case "Feb":
                return "02";
            case "Mar":
                return "03";
            case "Apr":
                return "04";
            case "May":
                return "05";
            case "Jun":
                return "06";
            case "Jul":
                return "07";
            case "Aug":
                return "08";
            case "Sep":
                return "09";
            case "Oct":
                return "10";
            case "Nov":
                return "11";
            case "Dec":
                return "12";
            default:
                return "";
        }
    }
}


