public class Arrays {
    public static void main(String[] args) {
        int[] arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        long s = 0;
        for (int i = 0; i < 10000000; i++) {
            for (int j = 0; j < arr.length; j++) {
                s += arr[j];
            }
        }
        System.out.println("Array sum: " + s);
    }
}
