class Foo1 {
    public static void main(String args[]) {
        int x = (int)(Math.random() * 42 + 1);
        int y = 6345;
        int c = 0;
        int d = 23456;
        int f = 0;
        System.out.println("HelloWorld");
        System.out.println("--------------------" );
        System.out.println("initial value ");
        System.out.println("random number x : " + x);
        System.out.println("x = " + x );
        System.out.println("y = " + y );
        System.out.println("c = " + c );
        System.out.println("d = " + d );
        System.out.println("f = " + f );
        System.out.println("--------------------" );

        c = x + y;
        d += c;
        System.out.println("c = x + y = " +x+ " + " + y + " = " + c);
        System.out.println("d = d + c = " + d );
        x = c / 2;
        System.out.println("x = c/2 = " + x );

        c = x * y;
        d +=c;
        System.out.println("c = x * y = " +x+ " * " + y + " = " + c);
        System.out.println("d = d + c = " + d );
        x = c/2;
        System.out.println("x = c/2 = " + x );

        c = x-y;
        d +=c ;
        System.out.println("c = x - y = " +x+ " - " + y + " = " + c);
        System.out.println("d = d + c = " + d );
        x = c/2;
        System.out.println("x = c/2 = " + x );

        c = x/y;
        d +=c ;
        System.out.println("c = x / y = " +x+ " / " + y + " = " + c);
        System.out.println("d = d + c = " + d );

        f = d + x + y + c ;
        System.out.println("f = " + (d) + " + " + (x) + " + " + (y) + " + " + (c) + " = " + f );
        System.out.println("Foo Test By WJY");
    }
}
